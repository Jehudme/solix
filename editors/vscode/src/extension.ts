import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind,
    Executable
} from 'vscode-languageclient/node';

let client: LanguageClient | undefined;

function findServerBinary(): { command: string; args: string[] } {
    const config = vscode.workspace.getConfiguration('solix');
    let customPath = config.get<string>('lsp.path');
    if (customPath) {
        customPath = customPath.trim().replace(/^["']+|["']+$/g, '');
    }

    if (customPath && customPath.length > 0) {
        const resolvedPath = path.resolve(customPath);
        if (fs.existsSync(resolvedPath)) {
            const basename = path.basename(resolvedPath);
            if (basename === 'solix' || basename === 'solix.exe') {
                return { command: resolvedPath, args: ['lsp'] };
            }
            return { command: resolvedPath, args: [] };
        }
        return { command: customPath, args: ['lsp'] };
    }

    // Default to 'solix' CLI command with 'lsp' subcommand
    return { command: 'solix', args: ['lsp'] };
}

export function activate(context: vscode.ExtensionContext) {
    const serverBinary = findServerBinary();

    const serverExecutable: Executable = {
        command: serverBinary.command,
        args: serverBinary.args,
        transport: TransportKind.stdio,
        options: {
            env: process.env
        }
    };

    const serverOptions: ServerOptions = {
        run: serverExecutable,
        debug: serverExecutable
    };

    const clientOptions: LanguageClientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'solix' },
            { scheme: 'untitled', language: 'solix' }
        ],
        synchronize: {
            fileEvents: [
                vscode.workspace.createFileSystemWatcher('**/*.slx'),
                vscode.workspace.createFileSystemWatcher('**/solix.json')
            ]
        }
    };

    client = new LanguageClient(
        'solix-lsp',
        'Solix Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
    context.subscriptions.push({
        dispose: () => {
            if (client) {
                client.stop();
            }
        }
    });
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
