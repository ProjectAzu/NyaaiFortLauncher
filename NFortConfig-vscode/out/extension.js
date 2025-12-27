"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;
const vscode = require("vscode");
const LANG_ID = "nfortconfig";
function activate(context) {
    const diagnosticCollection = vscode.languages.createDiagnosticCollection(LANG_ID);
    context.subscriptions.push(diagnosticCollection);
    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(doc => {
        if (doc.languageId === LANG_ID)
            refreshDiagnostics(doc, diagnosticCollection);
    }), vscode.workspace.onDidChangeTextDocument(e => {
        if (e.document.languageId === LANG_ID)
            refreshDiagnostics(e.document, diagnosticCollection);
    }), vscode.workspace.onDidCloseTextDocument(doc => diagnosticCollection.delete(doc.uri)));
    context.subscriptions.push(vscode.languages.registerDocumentFormattingEditProvider(LANG_ID, {
        provideDocumentFormattingEdits(document) {
            const fullRange = new vscode.Range(document.positionAt(0), document.positionAt(document.getText().length));
            const formatted = formatTopLevelProps(document.getText());
            return [vscode.TextEdit.replace(fullRange, formatted)];
        }
    }));
    context.subscriptions.push(vscode.commands.registerCommand("nfortconfig.formatDocument", async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor || editor.document.languageId !== LANG_ID)
            return;
        await vscode.commands.executeCommand("editor.action.formatDocument");
    }));
    for (const doc of vscode.workspace.textDocuments) {
        if (doc.languageId === LANG_ID)
            refreshDiagnostics(doc, diagnosticCollection);
    }
}
function deactivate() { }
function refreshDiagnostics(document, collection) {
    const text = document.getText();
    const { errors } = parsePropertiesSetDataLikeCpp(text, document);
    const diagnostics = errors.map(e => {
        const d = new vscode.Diagnostic(e.range, e.message, vscode.DiagnosticSeverity.Error);
        return d;
    });
    collection.set(document.uri, diagnostics);
}
/**
 * Mirrors your C++ ParsePropertiesSetData behavior:
 * - Skip whitespace
 * - Parse name until ':'
 * - Skip whitespace
 * - Expect '{'
 * - Capture value by brace depth (NO string/comment special handling)
 */
function parsePropertiesSetDataLikeCpp(text, document) {
    const props = [];
    const errors = [];
    const isWs = (ch) => /\s/.test(ch); // close enough to iswspace for editor use
    const skipWhitespace = (i) => {
        while (i < text.length && isWs(text[i]))
            i++;
        return i;
    };
    const cleanNameCppStyle = (raw) => {
        // Match: keep alphabetic, digits, punctuation; remove others (incl whitespace)
        // Using Unicode categories: L=letter, N=number, P=punctuation
        return raw.replace(/[^\p{L}\p{N}\p{P}]+/gu, "");
    };
    let index = 0;
    while (true) {
        index = skipWhitespace(index);
        if (index >= text.length)
            break;
        const nameStart = index;
        // Find colon
        let colonPos = -1;
        while (index < text.length) {
            if (text[index] === ":") {
                colonPos = index;
                break;
            }
            index++;
        }
        if (colonPos === -1) {
            // "Expected ':' but none was found."
            const range = new vscode.Range(document.positionAt(nameStart), document.positionAt(text.length));
            errors.push({
                message: "Expected ':' but none was found. Parsing aborted.",
                range
            });
            return { props, errors };
        }
        const rawName = text.slice(nameStart, colonPos);
        const cleanedName = cleanNameCppStyle(rawName);
        const nameRange = new vscode.Range(document.positionAt(nameStart), document.positionAt(colonPos));
        // Move past colon
        index = colonPos + 1;
        // Skip whitespace between ':' and '{'
        index = skipWhitespace(index);
        // Expect '{'
        if (index >= text.length || text[index] !== "{") {
            const range = new vscode.Range(document.positionAt(index), document.positionAt(Math.min(index + 1, text.length)));
            errors.push({
                message: `Expected '{' after property name '${cleanedName}'.`,
                range
            });
            return { props, errors };
        }
        // Consume '{'
        index++;
        const valueStart = index; // CapturedValue begins here
        let braceDepth = 1;
        while (index < text.length && braceDepth > 0) {
            const c = text[index];
            if (c === "{") {
                braceDepth++;
                index++;
            }
            else if (c === "}") {
                braceDepth--;
                index++; // consume '}'
            }
            else {
                index++;
            }
        }
        if (braceDepth !== 0) {
            // Missing closing brace
            const range = new vscode.Range(document.positionAt(Math.max(valueStart - 1, 0)), document.positionAt(text.length));
            errors.push({
                message: `Missing '}' for property '${cleanedName}'.`,
                range
            });
            return { props, errors };
        }
        // At this point, index is positioned AFTER the closing '}'.
        const valueEndExclusive = index - 1; // exclude the closing '}'
        const valueRange = new vscode.Range(document.positionAt(valueStart), document.positionAt(valueEndExclusive));
        props.push({
            rawName,
            cleanedName,
            nameRange,
            valueRange
        });
    }
    return { props, errors };
}
/**
 * Conservative formatter:
 * - Rewrites the *top-level* structure as:
 *     CleanName:
 *     {
 *        <original captured value unchanged>
 *     }
 *
 * We do NOT attempt to re-indent inside the captured value because doing so could
 * accidentally alter strings/escapes/meaning. The C++ parser treats it literally.
 */
function formatTopLevelProps(input) {
    // We can't access vscode document here, so do a lightweight parse without ranges.
    const text = input.replace(/\r\n/g, "\n").replace(/\r/g, "\n");
    const isWs = (ch) => /\s/.test(ch);
    const skipWhitespace = (i) => {
        while (i < text.length && isWs(text[i]))
            i++;
        return i;
    };
    const cleanName = (raw) => raw.replace(/[^\p{L}\p{N}\p{P}]+/gu, "");
    let i = 0;
    const parts = [];
    while (true) {
        i = skipWhitespace(i);
        if (i >= text.length)
            break;
        const nameStart = i;
        let colonPos = -1;
        while (i < text.length) {
            if (text[i] === ":") {
                colonPos = i;
                break;
            }
            i++;
        }
        if (colonPos === -1) {
            // give up: return original if malformed
            return input;
        }
        const rawName = text.slice(nameStart, colonPos);
        const cleaned = cleanName(rawName);
        i = colonPos + 1;
        i = skipWhitespace(i);
        if (i >= text.length || text[i] !== "{") {
            return input; // malformed, don't destroy
        }
        i++; // consume '{'
        const valueStart = i;
        let depth = 1;
        while (i < text.length && depth > 0) {
            const c = text[i];
            if (c === "{")
                depth++;
            else if (c === "}")
                depth--;
            i++;
        }
        if (depth !== 0)
            return input;
        const valueEndExclusive = i - 1; // exclude closing '}'
        const capturedValue = text.slice(valueStart, valueEndExclusive);
        // Emit formatted block. Only adds whitespace OUTSIDE capturedValue.
        parts.push(`${cleaned}:\n{\n${capturedValue}\n}\n`);
    }
    return parts.join("\n").replace(/\n{3,}/g, "\n\n");
}
//# sourceMappingURL=extension.js.map