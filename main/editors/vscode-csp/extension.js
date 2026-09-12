'use strict';

const vscode = require('vscode');
const childProcess = require('child_process');
const path = require('path');

let diagnostics;
let status;

function compiler() {
  return vscode.workspace.getConfiguration('csp').get('compilerPath', 'cspc');
}

function activeCsp() {
  const editor = vscode.window.activeTextEditor;
  if (!editor || !['csp', 'cpasm'].includes(editor.document.languageId)) {
    vscode.window.showWarningMessage('Open a C+ (.csp) or CP ASM (.cpsm) file first.');
    return undefined;
  }
  return editor.document;
}

function execute(args, cwd) {
  return new Promise((resolve) => {
    const environment = {...process.env, NO_COLOR: '1'};
    childProcess.execFile(compiler(), args, {cwd, env: environment},
      (error, stdout, stderr) => resolve({error, output: `${stdout}${stderr}`}));
  });
}

function parseDiagnostics(document, output) {
  const values = [];
  const expression = /^(.*?):(\d+):(\d+):\s*(error|warning|note)(?:\[[^\]]+\])?:\s*(.*)$/gm;
  for (const match of output.matchAll(expression)) {
    const line = Math.max(0, Number(match[2]) - 1);
    const column = Math.max(0, Number(match[3]) - 1);
    const severity = match[4] === 'error' ? vscode.DiagnosticSeverity.Error :
      match[4] === 'warning' ? vscode.DiagnosticSeverity.Warning : vscode.DiagnosticSeverity.Information;
    const range = new vscode.Range(line, column, line, column + 1);
    const item = new vscode.Diagnostic(range, match[5], severity);
    item.source = 'cspc';
    values.push(item);
  }
  diagnostics.set(document.uri, values);
  return values;
}

async function check(document = activeCsp(), quiet = false) {
  if (!document) return false;
  if (document.isDirty) await document.save();
  status.text = '$(sync~spin) C+ checking';
  const result = await execute(['check', document.fileName], path.dirname(document.fileName));
  const found = parseDiagnostics(document, result.output);
  status.text = result.error || found.some(value => value.severity === vscode.DiagnosticSeverity.Error)
    ? '$(error) C+ errors' : '$(check) C+ ready';
  if (!quiet) {
    if (result.error) vscode.window.showErrorMessage(`C+ check failed with ${found.length} diagnostic(s).`);
    else vscode.window.showInformationMessage('C+ checks passed.');
  }
  return !result.error;
}

function shellQuote(value) {
  return `"${String(value).replace(/"/g, '\\"')}"`;
}

async function terminalCommand(mode) {
  const document = activeCsp();
  if (!document) return;
  await document.save();
  const configuration = vscode.workspace.getConfiguration('csp');
  const extra = configuration.get('buildArguments', ['-O2']).map(shellQuote).join(' ');
  const extension = process.platform === 'win32' ? '.exe' : '';
  const output = path.join(path.dirname(document.fileName), path.basename(document.fileName, path.extname(document.fileName)) + extension);
  const terminal = vscode.window.createTerminal({name: 'C+ Build', cwd: path.dirname(document.fileName)});
  terminal.show();
  const invoke = process.platform === 'win32' ? '& ' : '';
  const build = `${invoke}${shellQuote(compiler())} build ${shellQuote(document.fileName)} ${extra} -o ${shellQuote(output)}`;
  const run = process.platform === 'win32'
    ? `${build}; if ($LASTEXITCODE -eq 0) { & ${shellQuote(output)} }`
    : `${build} && ${shellQuote(output)}`;
  terminal.sendText(mode === 'run' ? run : build, true);
}

async function formatCurrent() {
  const document = activeCsp();
  if (!document) return;
  await document.save();
  const result = await execute(['fmt', document.fileName], path.dirname(document.fileName));
  if (result.error) vscode.window.showErrorMessage(result.output.trim() || 'C+ formatting failed.');
}

async function showRepresentation(command, suffix, language) {
  const document = activeCsp();
  if (!document) return;
  await document.save();
  const output = `${document.fileName}.${suffix}`;
  const arguments = command === 'ast-json'
    ? [document.fileName, '--ast-json', output, '--check']
    : [command, document.fileName, '-o', output];
  const result = await execute(arguments, path.dirname(document.fileName));
  if (result.error) return vscode.window.showErrorMessage(result.output.trim());
  const generated = await vscode.workspace.openTextDocument(output);
  await vscode.languages.setTextDocumentLanguage(generated, language);
  await vscode.window.showTextDocument(generated, {preview: true, viewColumn: vscode.ViewColumn.Beside});
}

function activate(context) {
  diagnostics = vscode.languages.createDiagnosticCollection('csp');
  status = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 50);
  status.text = '$(check) C+ ready';
  status.tooltip = 'C+ compiler status';
  status.command = 'csp.check';
  status.show();

  context.subscriptions.push(
    diagnostics, status,
    vscode.commands.registerCommand('csp.check', () => check()),
    vscode.commands.registerCommand('csp.build', () => terminalCommand('build')),
    vscode.commands.registerCommand('csp.run', () => terminalCommand('run')),
    vscode.commands.registerCommand('csp.format', formatCurrent),
    vscode.commands.registerCommand('csp.showAst', () => showRepresentation('ast-json', 'ast.json', 'json')),
    vscode.commands.registerCommand('csp.showMir', () => showRepresentation('mir', 'mir', 'plaintext')),
    vscode.workspace.onDidSaveTextDocument((document) => {
      if (['csp', 'cpasm'].includes(document.languageId) && vscode.workspace.getConfiguration('csp').get('diagnostics.onSave', true))
        check(document, true);
    }),
    vscode.workspace.onDidCloseTextDocument((document) => diagnostics.delete(document.uri)),
    vscode.window.onDidChangeActiveTextEditor((editor) => {
      status.text = editor && ['csp', 'cpasm'].includes(editor.document.languageId) ? '$(check) C+ ready' : 'C+';
    })
  );
}

function deactivate() {}
module.exports = {activate, deactivate};
