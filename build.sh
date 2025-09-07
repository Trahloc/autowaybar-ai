#!/bin/bash

# autowaybar-ai build script
# AI-enhanced version of autowaybar with security hardening and modern C++20 patterns

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    
    if ! command_exists g++; then
        missing_deps+=("g++")
    fi
    
    if ! command_exists xmake; then
        missing_deps+=("xmake")
    fi
    
    if ! pkg-config --exists fmt; then
        missing_deps+=("libfmt-dev")
    fi
    
    if ! pkg-config --exists jsoncpp; then
        missing_deps+=("libjsoncpp-dev")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        echo ""
        echo "Install with:"
        echo "  Arch Linux: sudo pacman -S gcc xmake fmt jsoncpp"
        echo "  Ubuntu/Debian: sudo apt install build-essential xmake libfmt-dev libjsoncpp-dev"
        echo "  Fedora: sudo dnf install gcc-c++ xmake fmt-devel jsoncpp-devel"
        exit 1
    fi
    
    print_success "All dependencies found"
}

# Function to build with xmake
build_xmake() {
    print_status "Building with xmake..."
    
    if [ "$1" = "release" ]; then
        xmake f -m release
        xmake
        print_success "Release build completed"
    else
        xmake f -m debug
        xmake
        print_success "Debug build completed"
    fi
}

# Function to build manually
build_manual() {
    print_status "Building manually with g++..."
    
    local mode="$1"
    local flags="-std=c++20 -Wall -Wextra -Wpedantic"
    
    if [ "$mode" = "release" ]; then
        flags="$flags -O3 -march=native -DNDEBUG"
    else
        flags="$flags -g -O0 -DDEBUG"
    fi
    
    # Find all source files
    local sources=$(find src -name "*.cpp" | tr '\n' ' ')
    
    # Compile
    g++ $flags $sources -o autowaybar -lfmt -ljsoncpp
    
    print_success "Manual build completed"
}

# Function to install
install_binary() {
    local install_dir="$1"
    local binary_path=""
    
    # Find the binary
    if [ -f "build/linux/x86_64/release/autowaybar" ]; then
        binary_path="build/linux/x86_64/release/autowaybar"
    elif [ -f "autowaybar" ]; then
        binary_path="autowaybar"
    else
        print_error "Binary not found. Build first with: $0 build"
        exit 1
    fi
    
    print_status "Installing to $install_dir..."
    
    if [ "$install_dir" = "system" ]; then
        sudo cp "$binary_path" /usr/local/bin/
        print_success "Installed to /usr/local/bin/autowaybar"
    else
        mkdir -p ~/.local/bin
        cp "$binary_path" ~/.local/bin/
        print_success "Installed to ~/.local/bin/autowaybar"
        print_warning "Make sure ~/.local/bin is in your PATH"
    fi
}

# Function to clean
clean() {
    print_status "Cleaning build artifacts..."
    
    if [ -d "build" ]; then
        rm -rf build/
    fi
    
    if [ -f "autowaybar" ]; then
        rm -f autowaybar
    fi
    
    if [ -d ".xmake" ]; then
        rm -rf .xmake/
    fi
    
    print_success "Clean completed"
}

# Function to test
test_binary() {
    local binary_path=""
    
    if [ -f "build/linux/x86_64/release/autowaybar" ]; then
        binary_path="build/linux/x86_64/release/autowaybar"
    elif [ -f "autowaybar" ]; then
        binary_path="autowaybar"
    else
        print_error "Binary not found. Build first with: $0 build"
        exit 1
    fi
    
    print_status "Testing binary..."
    
    if "$binary_path" --help >/dev/null 2>&1; then
        print_success "Binary works correctly"
        echo ""
        echo "Usage:"
        "$binary_path" --help
    else
        print_error "Binary test failed"
        exit 1
    fi
}

# Function to show help
show_help() {
    echo "autowaybar-ai build script"
    echo ""
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo ""
    echo "Commands:"
    echo "  build [release|debug]  Build the project (default: release)"
    echo "  install [system|user]  Install binary (default: user)"
    echo "  clean                  Clean build artifacts"
    echo "  test                   Test the binary"
    echo "  deps                   Check dependencies"
    echo "  help                   Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 build              # Build release version"
    echo "  $0 build debug        # Build debug version"
    echo "  $0 install system     # Install to /usr/local/bin (requires sudo)"
    echo "  $0 install user       # Install to ~/.local/bin"
    echo "  $0 clean              # Clean build artifacts"
    echo "  $0 test               # Test the binary"
    echo ""
    echo "Dependencies:"
    echo "  - g++ (C++20 support)"
    echo "  - xmake (optional, for xmake build)"
    echo "  - fmt library"
    echo "  - jsoncpp library"
}

# Main script logic
main() {
    case "${1:-build}" in
        "build")
            check_dependencies
            build_xmake "${2:-release}"
            ;;
        "build-manual")
            check_dependencies
            build_manual "${2:-release}"
            ;;
        "install")
            install_binary "${2:-user}"
            ;;
        "clean")
            clean
            ;;
        "test")
            test_binary
            ;;
        "deps")
            check_dependencies
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "Unknown command: $1"
            echo ""
            show_help
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"
