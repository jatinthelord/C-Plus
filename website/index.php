<?php

declare(strict_types=1);

$registryPath = dirname(__DIR__) . DIRECTORY_SEPARATOR . 'packages' . DIRECTORY_SEPARATOR . 'csx' . DIRECTORY_SEPARATOR . 'index.json';
$registry = json_decode((string) file_get_contents($registryPath), true, 512, JSON_THROW_ON_ERROR);
$packages = $registry['packages'] ?? [];
$query = trim((string) ($_GET['q'] ?? ''));

if ($query !== '') {
    $needle = strtolower($query);
    $packages = array_values(array_filter($packages, static function (array $package) use ($needle): bool {
        return str_contains(strtolower($package['name'] . ' ' . $package['description']), $needle);
    }));
}

function h(string $value): string
{
    return htmlspecialchars($value, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
}

?>
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="theme-color" content="#1793d1">
  <meta name="description" content="C+ compiler, packages, libraries and documentation.">
  <title>CSP Foundation</title>
  <link rel="icon" type="image/svg+xml" href="assets/csp-file-icon.svg">
  <link rel="stylesheet" href="assets/site.css">
</head>
<body>
  <div id="archdev-navbar"><ul>
    <li><a href="index.php"><strong>C+</strong> CSP Foundation</a></li>
    <li><a href="packages/">Packages</a></li>
    <li><a href="libraries/">Libraries</a></li>
    <li><a href="source/">Source</a></li>
    <li><a href="releases/">Releases</a></li>
    <li><a href="../README.md">Documentation</a></li>
    <li><a href="download/">Download</a></li>
  </ul></div>

  <div id="content">
    <div id="content-left-wrapper"><div id="content-left">
      <section id="intro">
        <h1>CSP Foundation</h1>
        <p>C+ is a systems programming language with a native compiler, standard library, and CSX package registry.</p>
        <p class="readmore"><a href="../README.md">Read the documentation »</a></p>
      </section>
      <section id="news">
        <h2>News</h2>
        <div><h3><a href="releases/">CSP 0.2.0 compiler release</a></h3><p><strong>2026-09-04</strong></p><p>Build native programs from <code>.csp</code> files and inspect C, Rust, LLVM, or assembly output.</p></div>
        <div><h3><a href="libraries/">C+ system library</a></h3><p><strong>2026-09-04</strong></p><p>Use handwritten filesystem, networking, process, serialization, time, Linux, HTTP, and CUDA headers.</p></div>
      </section>
      <section id="pkg-updates">
        <h2>Packages<?php if ($query !== ''): ?> matching “<?= h($query) ?>”<?php endif; ?></h2>
        <table class="results"><thead><tr><th>Name</th><th>Version</th><th>Description</th></tr></thead><tbody>
<?php foreach ($packages as $package): ?>
          <tr><td class="pkg-name"><a href="packages/package.html?name=<?= rawurlencode($package['name']) ?>"><?= h($package['name']) ?></a></td><td><?= h($package['version']) ?></td><td class="wrap"><?= h($package['description']) ?></td></tr>
<?php endforeach; ?>
<?php if (!$packages): ?><tr class="empty"><td colspan="3">No packages matched your search.</td></tr><?php endif; ?>
        </tbody></table>
        <p class="readmore"><a href="packages/">View the complete package repository »</a></p>
      </section>
    </div></div>
    <div id="content-right">
      <section id="pkgsearch" class="box"><h2>Package Search</h2><form action="index.php" method="get"><label for="package-query">Search CSP packages</label><input id="package-query" name="q" type="search" value="<?= h($query) ?>" placeholder="package or feature"><input type="submit" value="Search"></form></section>
      <section id="nav-sidebar" class="box"><h3>CSP resources</h3><ul><li><a href="../README.md">Installation guide</a></li><li><a href="../syntax.md">Language syntax</a></li><li><a href="libraries/">Standard library</a></li><li><a href="source/">Browse source</a></li><li><a href="releases/">Release notes</a></li></ul></section>
      <section class="box"><h3>Get started</h3><pre><code>php -S localhost:8080 -t website
cspc hello.csp -o hello.exe</code></pre><p><a href="download/">Download CSP</a></p></section>
    </div>
  </div>
  <div id="footer"><p>CSP Foundation · <a href="../LICENSE">MIT licensed</a> · <a href="../README.md">Source and documentation</a></p></div>
</body>
</html>
