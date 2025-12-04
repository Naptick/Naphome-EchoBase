#!/bin/bash
#
# sync-with-upstream.sh
# Synchronizes Naphome-EchoBase with upstream Naphome-Korvo1 changes
#
# Usage:
#   ./sync-with-upstream.sh [--push]
#
# Options:
#   --push    Push changes to origin after sync
#   --help    Show this help message
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse arguments
PUSH_CHANGES=false
while [[ $# -gt 0 ]]; do
    case $1 in
        --push)
            PUSH_CHANGES=true
            shift
            ;;
        --help)
            echo "Synchronizes Naphome-EchoBase with upstream Naphome-Korvo1 changes"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --push    Push changes to origin after sync"
            echo "  --help    Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Helper functions
print_header() {
    echo -e "${BLUE}=== $1 ===${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Check if we're in a git repository
if ! git -C "$REPO_ROOT" rev-parse --git-dir > /dev/null 2>&1; then
    print_error "Not in a git repository"
    exit 1
fi

cd "$REPO_ROOT"

print_header "Naphome-EchoBase Upstream Synchronization"

# Verify remote setup
print_header "Verifying Remote Configuration"
if ! git remote get-url upstream > /dev/null 2>&1; then
    print_error "upstream remote not configured"
    echo "Add upstream remote with:"
    echo "  git remote add upstream https://github.com/Naptick/Naphome-Korvo1.git"
    exit 1
fi

upstream_url=$(git remote get-url upstream)
origin_url=$(git remote get-url origin)

echo "Origin:   $origin_url"
echo "Upstream: $upstream_url"
print_success "Remote configuration verified"

# Check for uncommitted changes
print_header "Checking Working Tree"
if ! git diff --quiet --exit-code; then
    print_warning "Unstaged changes detected"
    echo "Stash your changes before syncing:"
    echo "  git stash"
    exit 1
fi

if ! git diff --cached --quiet --exit-code; then
    print_warning "Staged changes detected"
    echo "Commit or stash staged changes before syncing:"
    echo "  git commit -m 'Message'"
    exit 1
fi

print_success "Working tree is clean"

# Fetch upstream
print_header "Fetching Upstream Changes"
if git fetch upstream main; then
    print_success "Fetched upstream/main"
else
    print_error "Failed to fetch upstream"
    exit 1
fi

# Show what will be merged
print_header "Upstream Changes"
upstream_commits=$(git log origin/main..upstream/main --oneline)
if [ -z "$upstream_commits" ]; then
    print_success "Already up to date with upstream"
    exit 0
fi

echo "$upstream_commits"
read -p "Merge these changes? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Sync cancelled"
    exit 0
fi

# Merge upstream
print_header "Merging Upstream Changes"
if git merge upstream/main; then
    print_success "Successfully merged upstream/main"
else
    print_warning "Merge conflicts detected"
    echo ""
    echo "Please resolve conflicts manually:"
    echo "  1. Edit conflicted files"
    echo "  2. Stage resolved files: git add ."
    echo "  3. Complete merge: git commit"
    echo ""
    echo "Conflicted files:"
    git diff --name-only --diff-filter=U
    exit 1
fi

# Build verification (optional)
print_header "Verifying Build"
if command -v idf.py &> /dev/null; then
    read -p "Run build verification? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        if idf.py build > /tmp/build.log 2>&1; then
            print_success "Build verification passed"
        else
            print_warning "Build verification failed (check /tmp/build.log)"
            tail -20 /tmp/build.log
        fi
    fi
fi

# Push if requested
if [ "$PUSH_CHANGES" = true ]; then
    print_header "Pushing Changes"
    if git push origin main; then
        print_success "Pushed to origin/main"
    else
        print_error "Failed to push to origin"
        exit 1
    fi
else
    print_header "Sync Complete"
    echo "Review changes with: git log origin/main..HEAD --oneline"
    echo "Push changes with:  git push origin main"
fi

print_header "Summary"
print_success "Naphome-EchoBase synchronized with upstream Korvo1"
echo ""
echo "Next steps:"
echo "  1. Test on M5 Echo Base hardware"
echo "  2. Verify board-specific functionality"
echo "  3. Check audio I2S configuration"
echo "  4. Run: audio_abstraction_print_board_info() to verify pins"
