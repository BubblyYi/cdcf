# How to contribute to CDCF

We welcome new contributors, following is the general rule to contribute to the project.

## Project hierarchy

```text
.
├── CMakeLists.txt
├── actor_system            ....... Library to build actor system
│   ├── CMakeLists.txt
│   ├── include             .......   Exported headers
│   └── src                 .......   Implementation and testing for actor system
├── demos                   ....... Folders holding demos
│   ├── CMakeLists.txt
│   └── hello_world         .......   Folders holding per demo
│       └── CMakeLists.txt
└── node_keeper             ....... Executable to manage nodes in cluster
    ├── CMakeLists.txt
    └── src                 .......   Implementation and testing for node keeper
```
## Branching Model

The project generally follows the [GitHub Flow](https://guides.github.com/introduction/flow/) workflow:

* The main branch is the Master branch, where all the nightly development happens and is subject to CI/CD.
* To add a new feature, create a separate "Feature Branch" from the Master.
* To merge a new feature branch into the Master, file a Pull Request.
* For external contributor, the best way is to fork the repo, and then file a Pull Request for your feature branch.

## Commit Message

Writing good commit logs is important.  A commit log should describe what
changed and why.  Follow these guidelines when writing one:

1. The first line should be 50 characters or less and contain a short
   description of the change prefixed with the name of the changed
   subsystem (e.g. "net: add localAddress and localPort to Socket").
2. Keep the second line blank.
3. Wrap all other lines at 72 columns.

A good commit log looks like this:

```
subsystem: explaining the commit in one line

Body of commit message is a few lines of text, explaining things
in more detail, possibly giving some background about the issue
being fixed, etc etc.

The body of the commit message can be several paragraphs, and
please do proper word-wrap and keep columns shorter than about
72 characters or so. That way `git log` will show things
nicely even when it is indented.
```

The header line should be meaningful; it is what other people see when they
run `git shortlog` or `git log --oneline`.

Check the output of `git log --oneline files_that_you_changed` to find out
what subsystem (or subsystems) your changes touch.

## Style Guide

To keep the source consistent, readable, diffable and easy to merge, we use a coding style as defined by the [google-styleguide](https://github.com/google/styleguide) project. All patches will be expected to conform to the style outlined [here](https://google.github.io/styleguide/cppguide.html). Use [.clang-format](https://github.com/google/googletest/blob/master/.clang-format) to check your formatting.