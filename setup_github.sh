#!/bin/bash

# Setup script for creating Naphome-Korvo1 GitHub repository
# This script initializes git and prepares the repository for pushing to GitHub

set -e

REPO_DIR="/Users/danielmcshan/GitHub/Naphome-Korvo1"
ORG="Naptick"
REPO_NAME="Naphome-Korvo1"

cd "$REPO_DIR"

echo "=== Setting up Naphome-Korvo1 repository ==="
echo ""

# Check if git is already initialized
if [ -d ".git" ]; then
    echo "Git repository already initialized"
else
    echo "Initializing git repository..."
    git init
fi

# Add all files
echo "Adding files to git..."
git add .

# Check if there are changes to commit
if git diff --staged --quiet; then
    echo "No changes to commit"
else
    echo "Creating initial commit..."
    git commit -m "Initial commit: Korvo1 LED and audio test firmware

- LED control with WS2812 RGB ring
- Audio playback with ES8388 codec
- Log sweep generator for audio testing
- MP3 decoder support (optional)"
fi

echo ""
echo "=== Repository setup complete ==="
echo ""
echo "Next steps:"
echo "1. Create the repository on GitHub:"
echo "   https://github.com/organizations/${ORG}/repositories/new"
echo "   Name: ${REPO_NAME}"
echo ""
echo "2. Then run:"
echo "   git remote add origin https://github.com/${ORG}/${REPO_NAME}.git"
echo "   git branch -M main"
echo "   git push -u origin main"
echo ""
echo "Or use GitHub CLI:"
echo "   gh repo create ${ORG}/${REPO_NAME} --public --source=. --remote=origin --push"
echo ""
