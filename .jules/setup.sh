#!/usr/bin/env bash
set -e

# Install Dolt (required by beads)
sudo bash -c 'curl -L https://github.com/dolthub/dolt/releases/latest/download/install.sh | sudo bash'

# Install beads CLI (system-wide - don't clone this repo into your project)
curl -fsSL https://raw.githubusercontent.com/steveyegge/beads/main/scripts/install.sh | bash

# Initialize beads if not already present (repo may ship with .beads/)
[ -d .beads ] || bd init
git config beads.role maintainer
