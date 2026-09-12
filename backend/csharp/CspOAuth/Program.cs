using System.Net;
using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;
using System.Web;

namespace CspOAuth;

internal static class Program
{
    public static async Task<int> Main(string[] args)
    {
        var options = Options.FromEnvironment();
        if (args.Contains("--help") || args.Contains("-h"))
        {
            Console.WriteLine("""
                C+ OAuth gateway
                Environment:
                  CSP_OAUTH_CLIENT_ID
                  CSP_OAUTH_CLIENT_SECRET
                  CSP_OAUTH_LISTEN      (default http://127.0.0.1:8787/)
                  CSP_WEBSITE_URL       (default http://127.0.0.1:8000/)
                  CSP_C_BACKEND_URL     (default http://127.0.0.1:8081/)
                Open /login to start GitHub OAuth. The gateway tells the website
                and the C backend when a session is created.
                """);
            return 0;
        }

        using var listener = new HttpListener();
        listener.Prefixes.Add(options.Listen);
        listener.Start();
        Console.WriteLine($"C+ OAuth gateway {options.Listen}");
        Console.WriteLine($"Website notify  {options.WebsiteUrl}");
        while (true)
        {
            var context = await listener.GetContextAsync();
            _ = Task.Run(() => HandleAsync(context, options));
        }
    }

    private static async Task HandleAsync(HttpListenerContext context, Options options)
    {
        try
        {
            var path = context.Request.Url?.AbsolutePath.TrimEnd('/') ?? "";
            if (path == "/login")
            {
                var state = Convert.ToHexString(System.Security.Cryptography.RandomNumberGenerator.GetBytes(16));
                var redirect = options.Listen.TrimEnd('/') + "/callback";
                var url =
                    "https://github.com/login/oauth/authorize?client_id=" +
                    Uri.EscapeDataString(options.ClientId) +
                    "&redirect_uri=" + Uri.EscapeDataString(redirect) +
                    "&scope=read:user&state=" + state;
                context.Response.Cookies.Add(new Cookie("csp_oauth_state", state) { HttpOnly = true, Path = "/" });
                context.Response.Redirect(url);
                context.Response.Close();
                return;
            }

            if (path == "/callback")
            {
                var query = HttpUtility.ParseQueryString(context.Request.Url?.Query ?? "");
                var code = query["code"] ?? "";
                var state = query["state"] ?? "";
                var expected = context.Request.Cookies["csp_oauth_state"]?.Value;
                if (string.IsNullOrEmpty(code) || state != expected)
                {
                    await WriteAsync(context, 400, "text/plain", "invalid OAuth callback");
                    return;
                }

                var profile = await ExchangeGitHubAsync(options, code);
                await NotifyAsync(options.WebsiteUrl.TrimEnd('/') + "/api/oauth/session", profile);
                await NotifyAsync(options.CBackendUrl.TrimEnd('/') + "/api/oauth/session", profile);
                var dest = options.WebsiteUrl.TrimEnd('/') + "/account/oauth.html?login=" +
                           Uri.EscapeDataString(profile.Login) + "&provider=github";
                context.Response.Redirect(dest);
                context.Response.Close();
                return;
            }

            await WriteAsync(context, 200, "application/json",
                """{"service":"csp-oauth","login":"/login","callback":"/callback"}""");
        }
        catch (Exception error)
        {
            await WriteAsync(context, 500, "text/plain", error.Message);
        }
    }

    private static async Task<GitHubProfile> ExchangeGitHubAsync(Options options, string code)
    {
        using var http = new HttpClient();
        http.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
        http.DefaultRequestHeaders.UserAgent.ParseAdd("CPlus-OAuth-Gateway");
        var redirect = options.Listen.TrimEnd('/') + "/callback";
        using var tokenResponse = await http.PostAsync(
            "https://github.com/login/oauth/access_token",
            new FormUrlEncodedContent(new Dictionary<string, string>
            {
                ["client_id"] = options.ClientId,
                ["client_secret"] = options.ClientSecret,
                ["code"] = code,
                ["redirect_uri"] = redirect
            }));
        var tokenJson = await tokenResponse.Content.ReadAsStringAsync();
        using var tokenDoc = JsonDocument.Parse(tokenJson);
        if (!tokenDoc.RootElement.TryGetProperty("access_token", out var tokenElement))
            throw new InvalidOperationException("GitHub did not return an access token");
        var token = tokenElement.GetString() ?? "";
        http.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", token);
        using var userResponse = await http.GetAsync("https://api.github.com/user");
        userResponse.EnsureSuccessStatusCode();
        using var userDoc = JsonDocument.Parse(await userResponse.Content.ReadAsStringAsync());
        return new GitHubProfile(
            userDoc.RootElement.GetProperty("id").ToString(),
            userDoc.RootElement.GetProperty("login").GetString() ?? "unknown");
    }

    private static async Task NotifyAsync(string url, GitHubProfile profile)
    {
        using var http = new HttpClient { Timeout = TimeSpan.FromSeconds(5) };
        var payload = JsonSerializer.Serialize(new
        {
            provider = "github",
            subject = profile.Id,
            login = profile.Login,
            created = DateTimeOffset.UtcNow
        });
        using var content = new StringContent(payload, Encoding.UTF8, "application/json");
        try
        {
            await http.PostAsync(url, content);
        }
        catch (Exception error)
        {
            Console.Error.WriteLine($"notify {url}: {error.Message}");
        }
    }

    private static async Task WriteAsync(HttpListenerContext context, int status, string type, string body)
    {
        var bytes = Encoding.UTF8.GetBytes(body);
        context.Response.StatusCode = status;
        context.Response.ContentType = type;
        context.Response.ContentLength64 = bytes.Length;
        await context.Response.OutputStream.WriteAsync(bytes);
        context.Response.Close();
    }
}

internal sealed record GitHubProfile(string Id, string Login);

internal sealed record Options(string ClientId, string ClientSecret, string Listen, string WebsiteUrl, string CBackendUrl)
{
    public static Options FromEnvironment() => new(
        Environment.GetEnvironmentVariable("CSP_OAUTH_CLIENT_ID") ?? "",
        Environment.GetEnvironmentVariable("CSP_OAUTH_CLIENT_SECRET") ?? "",
        Environment.GetEnvironmentVariable("CSP_OAUTH_LISTEN") ?? "http://127.0.0.1:8787/",
        Environment.GetEnvironmentVariable("CSP_WEBSITE_URL") ?? "http://127.0.0.1:8000/",
        Environment.GetEnvironmentVariable("CSP_C_BACKEND_URL") ?? "http://127.0.0.1:8081/");
}
