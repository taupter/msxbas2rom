# VSCode Scaffolding

## Purpose

Defines the behavior of generating the VSCode MSX-BASIC project scaffolding (`.vscode` folder with `launch.json`, `tasks.json`, and `debug.tcl`) into a caller-chosen directory or, by default, into the current project folder, including the create-or-report-already-initialized contract.

## Requirements

### Requirement: VSCode scaffolding can target a caller-chosen directory

The VSCode MSX-BASIC project scaffolding SHALL expose an operation that accepts a target directory and creates a `.vscode` folder inside it containing `launch.json`, `tasks.json`, and `debug.tcl`. When the target directory already contains a `.vscode` folder, or the folder cannot be created, the operation SHALL report failure and SHALL NOT overwrite or alter any pre-existing file.

#### Scenario: Scaffold into an empty target directory
- **WHEN** the operation is invoked with a target directory that does not yet contain a `.vscode` folder
- **THEN** a `.vscode` folder is created inside the target directory
- **AND** `launch.json`, `tasks.json`, and `debug.tcl` are written inside it
- **AND** the operation reports success

#### Scenario: Scaffold fails when the target is already initialized
- **WHEN** the operation is invoked with a target directory that already contains a `.vscode` folder
- **THEN** the operation reports failure
- **AND** the existing `.vscode` contents are left unchanged

#### Scenario: Scaffold fails when the target directory cannot host the folder
- **WHEN** the operation is invoked with a target directory whose `.vscode` folder cannot be created (for example, a missing parent directory)
- **THEN** the operation reports failure
- **AND** no partial `.vscode` scaffolding is reported as success

### Requirement: Default initialization targets the current project folder

The default, no-argument initialization of the VSCode scaffolding SHALL target the process current working directory (the project folder where the CLI runs), so that a project's `.vscode` scaffolding is created relative to where the command is executed.

#### Scenario: Default initialization creates scaffolding in the current directory
- **WHEN** a user runs the VSCode project initialization in a directory that does not yet contain a `.vscode` folder
- **THEN** the `.vscode` folder with `launch.json`, `tasks.json`, and `debug.tcl` is created in that current directory
- **AND** the CLI reports that the project was initialized successfully

#### Scenario: Default initialization reports already-initialized projects
- **WHEN** a user runs the VSCode project initialization in a directory that already contains a `.vscode` folder
- **THEN** the CLI reports that the project is already initialized
- **AND** no pre-existing file inside `.vscode` is modified
