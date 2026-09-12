# CSP Foundation website

This is a dependency-free static site. Open `index.html` directly or serve the
directory with any static HTTP server.

The PHP version of the homepage is `index.php`. It reads the CSX registry on
the server and supports package search with `index.php?q=network`. Apache uses
`.htaccess` to select this PHP page as the directory homepage:

```powershell
python tools/build_site_catalog.py
php -S localhost:8080 -t website
cspweb
cc -o cspapi backend/c/cspapi.c
.\cspapi.exe 8081
dotnet run --project backend/csharp/CspOAuth
```

```powershell
php -S localhost:8080 -t website
```

PHP is optional; the static `index.html` remains available when PHP is not
installed. The HTML and PHP versions use the same Arch-style navigation,
package tables, sidebar search, and repository links.

For the live library and compiler source browser, run the repository-backed
Compiled Go server from the project root:

```powershell
cspweb
```

Then open `http://127.0.0.1:8000/source/`. The server exposes read-only
`/api/libraries` and `/api/source?file=src/parser.cpp` endpoints and serves the
website from the same process.

Error routes are connected to the same CSP shell: missing pages use `404.html`,
unknown packages use `package-not-found.html`, invalid redirects use
`redirect-failed.html`, and server failures use `500.html`/`server-error.html`.
The informational `101.html` and unsupported-version `505.html` pages are also
available under `errors/`.

Release metadata lives in `assets/releases.js`. Download entries contain the
real byte size, URL, version, platform, and SHA-256 digest for each artifact.
The supplied release API record is also available as `assets/release.json` for
tools that need strict JSON rather than JavaScript.

The reusable C+ file icon is `assets/csp-file-icon.svg`. Use it as the favicon
or as the icon source when registering `.csp` files with an editor or desktop
environment.

Verify every published artifact from the repository root:

```powershell
.\tools\verify-release.ps1
```

The interface uses only local HTML, CSS, and JavaScript. It has no analytics,
external fonts, generated imagery, tracking scripts, or framework dependency.
