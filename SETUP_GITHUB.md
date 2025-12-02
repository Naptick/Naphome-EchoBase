# Setting Up GitHub Repository

This guide will help you create the GitHub repository in the Naptick organization.

## Option 1: Using GitHub CLI (Recommended)

If you have GitHub CLI installed:

```bash
cd /Users/danielmcshan/GitHub/Naphome-Korvo1

# Initialize git repository
git init
git add .
git commit -m "Initial commit: Korvo1 LED and audio test firmware"

# Create repository in Naptick organization
gh repo create Naptick/Naphome-Korvo1 --public --source=. --remote=origin --push
```

## Option 2: Using GitHub Web Interface

1. Go to https://github.com/organizations/Naptick/repositories/new
2. Repository name: `Naphome-Korvo1`
3. Description: "ESP32-S3 firmware for Korvo1 development board with LED control and audio playback"
4. Choose visibility (Public/Private)
5. **Do NOT** initialize with README, .gitignore, or license (we already have these)
6. Click "Create repository"

Then run:

```bash
cd /Users/danielmcshan/GitHub/Naphome-Korvo1

# Initialize git repository
git init
git add .
git commit -m "Initial commit: Korvo1 LED and audio test firmware"

# Add remote and push
git remote add origin https://github.com/Naptick/Naphome-Korvo1.git
git branch -M main
git push -u origin main
```

## Option 3: Manual Setup Script

Run this script to automate the setup:

```bash
cd /Users/danielmcshan/GitHub/Naphome-Korvo1
./setup_github.sh
```

## Verify Setup

After pushing, verify the repository:

```bash
git remote -v
git log --oneline
```

You should see:
- Remote pointing to `github.com/Naptick/Naphome-Korvo1.git`
- At least one commit with the initial files

## Next Steps

1. Add repository topics/tags on GitHub (e.g., `esp32`, `esp-idf`, `korvo1`, `audio`, `led`)
2. Set up GitHub Actions for CI/CD (optional)
3. Add license file if needed
4. Update README with any additional information
