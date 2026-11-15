/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

# ACP Client Plugin for Kate - Developer Guide

This document provides an overview of the Agent Client Protocol (ACP) client plugin for Kate, its architecture, and guidelines for development.

**Note:** Some parts of this plugin were adapted from the LSP Client plugin.

## Overview

The ACP Client plugin enables Kate to communicate with ACP-compliant agent servers (like Mistral Vibe, Claude Code, etc.) using the [Agent Client Protocol v1](https://agentclientprotocol.com).

The plugin implements the ACP v1 specification with JSON-RPC 2.0 over stdin/stdout communication.

### Features

- Interactive chat interface with ACP agents
- Session management (create, load, resume, close, delete, list)
- Tool call support with permission management
- Progress and usage tracking
- Server configuration and management
- Session list with context menu
- Message input widget with proper state management
- Agent mode and model selectors (ACP config options, with legacy fallbacks)
- Slash command menu and inline completion populated from agent-advertised commands
- Support for all ACP v1 session update types

### Supported ACP v1 Methods

#### Client-to-Server Requests:
- `initialize` - Initialize connection and negotiate capabilities
- `authenticate` - Authenticate with auth provider (ACP v1 correct method name)
- `logout` - End authenticated state (if agent supports `auth.logout`)
- `session/new` - Create new conversation session
- `session/load` - Load existing session (if agent supports `loadSession`)
- `session/resume` - Resume paused session (if agent supports `sessionCapabilities.resume`)
- `session/close` - Close active session (if agent supports `sessionCapabilities.close`)
- `session/delete` - Delete session (if agent supports `sessionCapabilities.delete`)
- `session/list` - List all available sessions
- `session/prompt` - Send message to session
- `session/set_mode` - Set session mode (legacy API)
- `session/set_model` - Set session model (legacy API)
- `session/set_config_option` - Set a session configuration option (e.g. mode or model)
- `session/cancel` - Cancel current operation (**notification**, not a method)
- `session/request_permission` - Request user permission for tool calls
- `tools/list` - List available tools
- `tools/call` - Call a specific tool

#### Server-to-Client Notifications:
- `session/update` - Session state updates (streaming responses)
- `$/progress` - Progress notifications for long-running operations
- `$/cancel_request` - Cancellation requests from agent
- `session/request_permission` - Permission requests for tool calls
- `elicitation/complete` - Elicitation complete notification

#### Session Update Types:
- `user_message_chunk` - Streaming user message (echo)
- `agent_message_chunk` - Streaming agent response
- `agent_thought_chunk` - Streaming agent reasoning
- `tool_call` - Tool call notification
- `tool_call_update` - Tool call progress/update
- `plan` - Agent execution plan
- `available_commands` - Available slash commands update
- `current_mode_update` - Session mode change
- `config_option_update` - Session configuration update
- `session_info_update` - Session metadata update
- `usage_update` - Token/cost usage information
- `notice` - User-facing notice (requires `session.notices` capability)
- `compaction_update` - Context compaction (requires `session.compaction` capability)
- `compaction_summary_chunk` - Compaction summary (requires `session.compaction` capability)

## Architecture

### Component Hierarchy

```
ACPClientPlugin (KTextEditor::Plugin)
├── ACPClientPluginView (KXMLGUIClient)
│   ├── QTabWidget (main container)
│   │   ├── ACPServerListWidget - Manage ACP server configurations
│   │   ├── ACPSessionListWidget - List, resume, delete sessions
│   │   │   ├── Context menu (Resume, Delete)
│   │   │   ├── New Session button
│   │   │   └── Reload button
│   │   └── ACPClientChatWidget - Chat interface
│   │       ├── QTextEdit (message input, disabled when no session)
│   │       ├── Agent mode selector (hidden unless the agent exposes modes)
│   │       ├── Model selector (hidden unless the agent exposes models)
│   │       ├── Commands button (hidden unless the agent exposes commands)
│   │       ├── Send button
│   │       ├── New Session button
│   │       ├── End Session button
│   │       └── ACPChatMessageWidget list (conversation history)
│   └── Actions (menu/toolbar items)
│
└── ACPClientServerManager (Singleton QObject)
    └── ACPClientServer (QObject, per-server)
        └── QProcess (server connection, stdin/stdout)
```

### Core Files

| File | Purpose |
|------|---------|
| `acpclientplugin.h/.cpp` | Main plugin class, configuration management, plugin lifecycle |
| `acpclientpluginview.h/.cpp` | View integration, tool view management, action handling |
| `acpclientservermanager.h/.cpp` | Manages multiple server connections, session lifecycle, capability checks |
| `acpclientserver.h/.cpp` | Individual server connection, JSON-RPC message parsing, state management |
| `acpclientprotocol.h/.cpp` | ACP v1 protocol implementation, message creation/parsing, constants |
| `acpclientchatwidget.h/.cpp` | Chat UI widget, message display, user input handling |
| `acpchatmessagewidget.h/.cpp` | Individual message widget with rich formatting, styling |
| `acpcollapsibletextwidget.h/.cpp` | Collapsible text block used for long tool call input and output |
| `acpclientconfigpage.h/.cpp` | Configuration page for Kate settings |
| `acpserverlistwidget.h/.cpp` | Server list management UI |
| `acpsessionlistwidget.h/.cpp` | Session list management UI with context menu |
| `acpclient_debug.h` | Debug logging category |

### UI Files

| File | Purpose |
|------|---------|
| `acpclientchat.ui` | Chat widget UI (input, display, buttons) |
| `acpconfigwidget.ui` | Configuration widget UI |
| `ui.rc` | KPart GUI XML (menus, toolbars) |
| `plugin.qrc` | Qt resource file |
| `settings.json` | Default server configurations |

## Data Flow

### Message Flow: User → Agent

```
User types message in QTextEdit
    ↓
ACPClientChatWidget::sendMessage() - validates session exists
    ↓
ACPClientChatWidget::onInputTextChanged() - enables/disables send button
    ↓ (Ctrl+Enter or Send button click)
ACPClientChatWidget::sendMessage()
    ↓
ACPClientServerManager::sendPrompt(sessionId, message)
    ↓
ACPClientServer::sendMessage() [session/prompt with ContentBlock array]
    ↓
Agent processes request and sends responses via session/update notifications
```

### Message Flow: Agent → User

```
ACPClientServer receives JSON-RPC message from agent
    ↓
ACPClientServer::messageReceived() signal emitted
    ↓
ACPClientServerManager::onServerMessageReceived()
    ↓
Routes based on message type:
    - session/update → ACPClientServerManager::sessionUpdateReceived() signal
    - session/request_permission → ACPClientServerManager::permissionRequested() signal
    - $/progress → ACPClientServerManager::messageReceived() signal
    - $/cancel_request → ACPClientServerManager::messageReceived() signal
    ↓
ACPClientChatWidget receives signals and updates UI:
    - onServerMessageReceived() → displays agent messages, tool calls, plans
    - onPermissionRequested() → shows permission dialog
    - onProgressNotification() → updates progress bar
    ↓
ACPChatMessageWidget renders individual messages with proper styling
```

### Session Lifecycle

```
User clicks "New Session" button
    ↓
ACPSessionListWidget::newSessionRequested() signal
    ↓
ACPClientPluginView::setupUI() connects to signal
    ↓
ACPClientChatWidget::startNewSession()
    ↓
Sets up server connections, connects to sessionCreated signal
    ↓
ACPClientServerManager::createSession()
    ↓
ACPClientServer::sendMessage() [session/new with cwd, mcpServers]
    ↓
Agent responds with sessionId
    ↓
ACPClientServerManager::sessionCreated() signal
    ↓
ACPClientChatWidget sets sessionId, displays "New session started"
    ↓
User can now send messages
```

## ACP Protocol Implementation

The plugin implements ACP Protocol v1 (JSON-RPC 2.0 based) as defined in `acpclientprotocol.h` and follows the [ACP v1 specification](https://agentclientprotocol.com).

### Protocol Constants

All protocol methods, notifications, and constants are defined in the `ACP` namespace in `acpclientprotocol.h`:

- **Methods**: `ACP::methodInitialize()`, `ACP::methodAuthenticate()`, `ACP::methodSessionNew()`, etc.
- **Notifications**: `ACP::notificationSessionUpdate()`, `ACP::notificationProgress()`, etc.
- **Session Update Types**: `ACP::sessionUpdateAgentMessageChunk()`, `ACP::sessionUpdatePlan()`, etc.
- **Content Types**: `ACP::contentTypeText()`, `ACP::contentTypeResource()`, etc.
- **Permission Types**: `ACP::permissionKindAllowOnce()`, `ACP::permissionKindAllowAlways()`, etc.

### Supported Methods (Client → Server)

All methods are implemented in `ACPProtocol` class with corresponding `create*Request()` or `create*Notification()` methods:

- `initialize` - Initialize connection, negotiate protocol version and capabilities
- `authenticate` - Authenticate with auth provider (correct ACP v1 method name)
- `logout` - End authenticated state (if agent supports `auth.logout`)
- `session/new` - Create new conversation session
- `session/load` - Load existing session (if agent supports `loadSession`)
- `session/resume` - Resume paused session (if agent supports `sessionCapabilities.resume`)
- `session/close` - Close active session (if agent supports `sessionCapabilities.close`)
- `session/delete` - Delete session (if agent supports `sessionCapabilities.delete`)
- `session/list` - List all available sessions
- `session/prompt` - Send prompt/message to a session
- `session/cancel` - Cancel current operation (**notification**, not a request-response method)
- `session/request_permission` - Request user permission for tool calls
- `session/set_mode` - Set session mode (legacy API)
- `session/set_model` - Set session model (legacy API)
- `session/set_config_option` - Set a session configuration option (e.g. mode or model)
- `tools/list` - List available tools
- `tools/call` - Call a specific tool

### Supported Notifications (Server → Client)

- `session/update` - Session state updates (agent messages, tool calls, plans, etc.)
- `$/progress` - Progress notifications for long-running operations
- `$/cancel_request` - Cancellation requests from agent
- `session/request_permission` - Permission requests for tool calls
- `elicitation/complete` - Elicitation complete notification

### Session Management

The plugin checks agent capabilities from the `initialize` response to determine which session methods are available:

- **session/new** - Always available after initialization
- **session/load** - Used if `agentCapabilities.loadSession` is true (loads session with full conversation replay)
- **session/resume** - Used if `agentCapabilities.sessionCapabilities.resume` is true (resumes session without replay)
- **session/close** - Used if `agentCapabilities.sessionCapabilities.close` is true
- **session/delete** - Used if `agentCapabilities.sessionCapabilities.delete` is true
- **session/list** - Used to list all available sessions

The `ACPClientServerManager` class provides capability check methods:
- `supportsLoadSession()` - Check if agent supports session/load
- `supportsResumeSession()` - Check if agent supports session/resume
- `supportsCloseSession()` - Check if agent supports session/close
- `supportsDeleteSession()` - Check if agent supports session/delete
- `supportsSetMode()` - Check if agent supports session/set_mode
- `supportsLogout()` - Check if agent supports logout

### Session Update Types

All session update types from ACP v1 spec are defined in `acpclientprotocol.h`:

- `agent_message_chunk` - Streaming agent message
- `user_message_chunk` - Streaming user message (echo)
- `agent_thought_chunk` - Streaming agent reasoning
- `plan` - Agent execution plan
- `tool_call` - Tool call notification
- `tool_call_update` - Tool call progress/update
- `usage_update` - Token/cost usage information
- `mode` / `current_mode_update` - Mode changes
- `available_commands` / `available_commands_update` - Available slash commands
- `session_info_update` - Session metadata update
- `notice` - User-facing notice (requires `session.notices` capability)
- `compaction_update` - Context compaction (requires `session.compaction` capability)
- `compaction_summary_chunk` - Compaction summary (requires `session.compaction` capability)
- `config_option_update` - Session configuration update

### Session Configuration Selectors

The chat widget shows two dropdowns above the conversation when the agent
advertises the corresponding settings:

- **Agent** - the session mode (for example `build` or `plan`)
- **Model** - the model the agent should use

Modern agents expose both through `configOptions` in the `session/new`,
`session/load` and `session/resume` responses. Options with
`category: "mode"` populate the Agent selector; options with
`category: "model"` (or the ID/name `model`) populate the Model selector.
Selecting a value sends `session/set_config_option` and the agent's new state
arrives through `config_option_update` or the request result.

If no `configOptions` are present, the client falls back to the legacy
`modes` and `models` response fields and sends `session/set_mode` or
`session/set_model`. Current selections are kept in sync through
`current_mode_update`. Per the ACP specification, legacy `modes` are ignored
once an agent sends `configOptions`.

### Slash Commands

Agents advertise slash commands through the `available_commands_update`
session update (and the legacy `available_commands` update). Each command has
a `name`, an optional `description` and an optional `input.hint` describing
the expected arguments.

When at least one command is available, the chat widget shows a `/` button
next to the message input. The button opens a menu listing the commands with
their descriptions as tooltips. Selecting a command replaces the input text
with `/name ` and focuses the input so the user can add arguments and send
the prompt. The command list is cleared when the session changes and the
button is only enabled while a session is active.

The message input also completes commands while typing. When the input starts
with `/` and the cursor is still inside the command token, a `QCompleter`
popup suggests matching command names. Accepting a suggestion replaces the
token with `/name ` and keeps any text that follows it. Completion stops as
soon as the cursor moves past the command token, so arguments are never
filtered against command names.

Note: Qt Creator's chat input uses the same behavior, but its implementation
is tied to Qt Creator's `TextEditor` completion framework and cannot be
copied into Kate. The Kate version only shares the behavior, implemented with
`QCompleter` against the `QTextEdit` input widget.

## Message Widget Types

`ACPChatMessageWidget` supports these message types (see `MessageType` enum):

- **User** - User's messages (blue styling)
- **Agent** - Agent's text responses (green styling)
- **System** - System messages (gray styling)
- **Plan** - Plan/step updates (blue border, structured display)
- **ToolCall** - Tool call notifications (orange styling)
- **ToolCallUpdate** - Tool call progress (blue styling)
- **Usage** - Usage/token information (purple styling)
- **PermissionRequest** - Inline permission prompts with buttons for all permission options (allow_once, allow_always, reject_once, reject_always)

### Collapsing Long Tool Call Input and Output

Tool call input (the command) and output are displayed with
`ACPCollapsibleTextWidget`. Text longer than 10 lines, or longer than 2000
characters, is collapsed to a preview: the first two lines, a button stating
how many lines are hidden, and the last two lines. Text with few but very long
lines is previewed by characters instead, since it has no useful last lines.

Both the header and the separator are `QToolButton`s, so the text can be
expanded and collapsed with the mouse or the keyboard, and the collapsed state
is visible rather than implied. Preview lines are also shortened to 400
characters so a single huge line cannot blow up the message height.

The tool call title is treated the same way. `title` is required by the
protocol and agents commonly put the whole shell command into it, which the
input block then repeats, so the title is whitespace-normalized and displayed
in a `KSqueezedTextLabel` that elides to the available width and is capped at
roughly 60 characters. `KSqueezedTextLabel` manages its own tooltip, so the
tooltip holds the normalized title rather than the verbatim one; the verbatim
command remains in the input block.

`ACPChatMessageWidget` rebuilds its content widgets on every tool call update,
so the expansion state is stored in the message widget (`m_toolCommandExpanded`
and `m_toolContentExpanded`) and reapplied. Toggling emits
`ACPChatMessageWidget::contentSizeChanged()`, which `ACPClientChatWidget` uses
to re-measure the `QListWidgetItem` size hint. Without that, the expanded text
would be clipped by the stale item height.

## Configuration

### Plugin Options

Stored in KConfig (`acpclient` group):

- `AutoStartSession` - Start session automatically on Kate startup
- `ShowNotifications` - Show ACP notifications in Kate
- `ShowToolCalls` - Display tool call details in chat
- `ShowProgress` - Display progress updates
- `DebugMode` - Enable debug logging
- `ToolCallPermission` - Permission mode: AllowAll, DenyAll, AskEachTime
- `AllowedServerCommandLines` - Whitelist of allowed server commands
- `BlockedServerCommandLines` - Blacklist of blocked server commands

### Server Configuration

Stored in `~/.config/kate/acpclient/settings.json`:

```json
{
  "servers": {
    "Mistral Vibe": {
      "command": "vibe-acp",
      "arguments": [],
      "version": "1.0",
      "description": "Mistral AI's Vibe CLI with ACP support",
      "repository": "https://github.com/mistralai/vibe",
      "website": "https://vibe.mistral.ai/"
    }
  }
}
```

Note: The `servers` field is an object where each key is the server name.

## Development Guidelines

### Code Formatting

Before committing changes, please run the following to ensure consistent code formatting:

```bash
git clang-format --force
```

This will automatically format all modified C++ files according to the project's coding style (KDE coding standards).

### Adding New Features

When adding new ACP protocol features:

1. **New Methods/Notifications**:
   - Add method/notification constant to `acpclientprotocol.h` (e.g., `methodNewFeature()`)
   - Add creation method to `ACPProtocol` class (e.g., `createNewFeatureRequest()`)
   - Add parsing support if needed
   - Check agent capabilities before using

2. **New Session Update Types**:
   - Add update type constant (e.g., `sessionUpdateNewType()`)
   - Add handling in `ACPClientChatWidget::onServerMessageReceived()`
   - Add display logic in `ACPChatMessageWidget`

3. **New Capabilities**:
   - Add to `ClientCapabilities` or `AgentCapabilities` struct in `acpclientprotocol.h`
   - Add capability check method to `ACPClientServerManager` if needed
   - Update initialization to advertise client capabilities

4. **New UI Elements**:
   - Add to appropriate widget (chat, session list, server list)
   - Connect signals and slots properly
   - Ensure proper enabled/disabled state based on session/server availability

### Debugging

Enable debug output:

```bash
# Enable all ACP client debug messages
ACPCLIENT_DEBUG=1 kate

# Or via environment variable
export ACPCLIENT_DEBUG=1
```

Debug output uses the `kateacpclientplugin` logging category.

### Testing

The plugin compiles as part of Kate's build system. To test:

```bash
cd /data/home/sandbox/projects/kde/build/kate
cmake --build . -j4
kate
```

Then enable the plugin via:
- Settings → Configure Kate → Plugins → ACP Client
- Or via Kate's plugin menu

#### Testing with vibe-acp

1. Ensure `vibe-acp` is installed and in PATH
2. Start Kate
3. Open ACP Chat tool view (View → Tool Views → ACP Chat)
4. Click "New Session" button or use session list
5. Type messages in the input field (disabled when no session is active)
6. Use Ctrl+Enter to send messages

### Known Issues & TODOs

| Location | Issue | Priority |
|----------|-------|----------|
| Session compaction | Not fully implemented | Low |
| System prompt | Structure in place but not used in session requests | Low |
| Elicitation | Capability structure added but not implemented | Low |
| MCP HTTP/SSE transport | Capabilities defined but not implemented | Low |
| `acpclientpluginview.cpp:151` | Implement session manager dialog | Medium |
| `acpclientpluginview.cpp:157` | Implement server configuration dialog | Medium |
| `acpclientpluginview.cpp:162` | Implement tool palette | Low |
| `acpclientplugin.cpp:106` | `message->show()` call is commented out | Low |

## File Structure

```
addons/acpclient/
├── CMakeLists.txt           # Build configuration
├── AGENTS.md               # This file
├── plugin.qrc              # Qt resources
├── settings.json           # Default server configs
├── ui.rc                   # KPart GUI XML
│
├── Headers (.h):
│   ├── acpclientplugin.h
│   ├── acpclientpluginview.h
│   ├── acpclientservermanager.h
│   ├── acpclientserver.h
│   ├── acpclientprotocol.h
│   ├── acpclientchatwidget.h
│   ├── acpchatmessagewidget.h
│   ├── acpclientconfigpage.h
│   ├── acpserverlistwidget.h
│   ├── acpsessionlistwidget.h
│   └── acpclient_debug.h
│
├── Source (.cpp):
│   ├── acpclientplugin.cpp
│   ├── acpclientpluginview.cpp
│   ├── acpclientservermanager.cpp
│   ├── acpclientserver.cpp
│   ├── acpclientprotocol.cpp
│   ├── acpclientchatwidget.cpp
│   ├── acpchatmessagewidget.cpp
│   ├── acpclientconfigpage.cpp
│   ├── acpserverlistwidget.cpp
│   └── acpsessionlistwidget.cpp
│
└── UI Files (.ui):
    ├── acpclientchat.ui
    ├── acpconfigwidget.ui
    └── acpserverdialog.ui
```

## Key Classes

### ACPClientPlugin

Main plugin class that:
- Manages plugin lifecycle
- Stores configuration
- Creates views for each Kate main window
- Provides access to server manager
- Handles command line permission requests

### ACPClientPluginView

View class that:
- Integrates with Kate's GUI (menus, toolbars)
- Creates and manages the chat tool view
- Handles action triggers
- Manages connections between UI and server manager

### ACPClientServerManager

Singleton that:
- Manages multiple server connections
- Handles session lifecycle
- Routes messages between servers and UI
- Manages permission requests
- Provides capability checking methods

### ACPClientServer

Individual server connection that:
- Manages QProcess for server I/O
- Handles JSON-RPC message parsing
- Tracks request/response pairs
- Emits signals for server events

### ACPClientChatWidget

Chat UI widget that:
- Displays conversation history
- Handles user input
- Shows agent responses, tool calls, plans
- Manages permission request UI
- Tracks session state

### ACPChatMessageWidget

Individual message widget that:
- Displays formatted messages with timestamps
- Handles different message types with appropriate styling
- Supports inline permission requests with buttons

## Protocol Notes

- The plugin uses JSON-RPC 2.0 with ACP-specific methods
- Protocol version is negotiated during `initialize`
- Messages are newline-delimited JSON over stdin/stdout
- Each message is parsed and routed based on its type
- Request IDs can be integers or strings (handled by `QJsonValue`)

## Useful Patterns

### Sending a Request

```cpp
ACP::SomeParams params;
// ... set params ...
qint64 requestId = ACP::ACPProtocol::generateRequestId();
QJsonDocument request = ACP::ACPProtocol::createSomeRequest(params, requestId);
server->sendMessage(request);
```

### Handling a Response

```cpp
// In server or server manager
connect(server, &ACPClientServer::messageReceived, this, [this](const QJsonDocument &doc) {
    ACP::ACPMessage message;
    if (ACP::ACPProtocol::parseMessage(doc, message)) {
        if (message.isResponse) {
            // Handle response
        } else if (message.isNotification) {
            // Handle notification
        }
    }
});
```

### Adding a New Message Type

```cpp
// 1. Add to MessageType enum in acpchatmessagewidget.h
// 2. Add case to updateContentDisplay() in acpchatmessagewidget.cpp
// 3. Add styling in getTypeStyle()
// 4. Add label in getTypeLabel()
```

## Resources

- ACP Protocol Specification: https://agentclientprotocol.com
- Mistral Vibe: https://github.com/mistralai/vibe
- Kate Plugin Development: https://docs.kde.org/stable5/en/katepart/katepart/dev-plugins.html
