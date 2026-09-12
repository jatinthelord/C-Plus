(() => {
  const data = window.CSP_BENCHMARK_DATA;
  const table = document.querySelector('#benchmark-results');
  const summary = document.querySelector('#benchmark-summary');
  if (!data || !table || !summary) return;
  const escape = value => String(value).replace(/[&<>"']/g, character => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[character]));
  const milliseconds = value => `${Number(value).toFixed(3)} ms`;
  const bytes = value => value > 1048576 ? `${(value / 1048576).toFixed(2)} MiB` : `${(value / 1024).toFixed(1)} KiB`;
  table.innerHTML = data.results.map(item => item.status === 'ok' ? `<tr><td class="pkg-name">${escape(item.language)}</td><td><strong>${milliseconds(item.median_ms)}</strong></td><td>${milliseconds(item.min_ms)}</td><td>${milliseconds(item.max_ms)}</td><td>${milliseconds(item.compile_ms)}</td><td>${bytes(item.binary_bytes)}</td></tr>` : `<tr><td>${escape(item.language)}</td><td colspan="5">${escape(item.status)}</td></tr>`).join('');
  const ranked = data.results.filter(item => item.status === 'ok').sort((a, b) => a.median_ms - b.median_ms);
  const fastest = ranked[0];
  const close = fastest ? ranked.filter(item => item.median_ms <= fastest.median_ms * 1.02) : [];
  const observation = close.length > 1 ? `${close.map(item => escape(item.language)).join(', ')} within 2%` : fastest ? `${escape(fastest.language)} at ${milliseconds(fastest.median_ms)}` : 'No completed results';
  summary.innerHTML = `<h2>Latest local run</h2><dl class="benchmark-meta"><div><dt>Measured</dt><dd>${escape(data.generated_at)}</dd></div><div><dt>Machine</dt><dd>${escape(data.machine.cpu)}</dd></div><div><dt>Work</dt><dd>${Number(data.method.rounds).toLocaleString()} iterations</dd></div><div><dt>Observed result</dt><dd>${observation}</dd></div></dl><p class="benchmark-warning">Differences this small can be normal run-to-run noise. This is not a universal language ranking.</p>`;
})();
