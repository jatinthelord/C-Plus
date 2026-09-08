# C+ VS Code file icon

This folder contains the official VS Code extension for C+ source files.

It provides syntax highlighting, snippets, bracket and fold rules, on-save
compiler diagnostics, a status bar, and commands for checking, formatting,
building, running, AST inspection, and MIR inspection.

## Install locally

```powershell
code --install-extension csp-language-0.2.0.vsix --force
```

Restart VS Code, open a `.csp` file, and use the `C+` commands from the Command
Palette. Set `csp.compilerPath` if `cspc` is not on `PATH`.

Publishing publicly requires the CSP Foundation Visual Studio Marketplace
publisher account and its access token.
