# AI-CONTEXT.md
# Project-specific context for autowaybar-ai

This file contains the specific metrics, targets, and implementation details for the autowaybar-ai project. 

**The generic anti-kruft principles are in `.cursorrules` - this file contains only project-specific details and metrics.**

## PROJECT OVERVIEW
**What this tool actually does:**
1. Poll mouse position every 80ms
2. Modify waybar config JSON
3. Send signals to waybar process
4. That's it. Keep it simple.

## PROJECT-SPECIFIC PERFORMANCE TARGETS
- **Startup time: < 20ms** - Users expect instant response for a daemon
- **Memory usage: < 1MB** - It's a simple tool that should be lightweight
- **Binary size: < 200KB** - No bloat allowed for a system utility
- **Runtime CPU cycles: MINIMIZE** - Cache expensive operations at startup, optimize for daemon lifetime
- **Kernel scheduling: Low priority** - Let kernel manage CPU allocation, focus on cycle efficiency

## PROJECT-SPECIFIC SUCCESS METRICS
- **Lines of code: < 1000** - If it's longer, it's too complex (excludes comments)
- **Files: < 10** - If you need more, the design is wrong
- **Dependencies: < 3** - Every dependency is a liability (fmt, jsoncpp)
- **Build time: < 5 seconds** - Fast feedback loop
- **User can understand the code** - If they can't, it's too complex

## PROJECT-SPECIFIC CODE COMPLEXITY LIMITS
- **Maximum 3 levels of indirection** - If you need more, the design is wrong
- **Maximum 50 lines per function** - If longer, break it down
- **Maximum 5 parameters per function** - If more, use a struct
- **No more than 2 abstraction layers** - Direct is better than "clean"

## PROJECT-SPECIFIC SYSTEM TOOLS
- **Use system tools** - Don't reinvent `pidof`, `hyprctl`, etc.
- **Trust the environment** - If `$HOME` is wrong, that's the user's problem
- **No XDG compliance** - Use `~/.config` and be done with it

## PROJECT-SPECIFIC TESTING
- **Test the happy path** - Does it work when everything is correct?
- **Test the obvious failures** - What happens when waybar isn't running?
- **No unit tests for trivial functions** - Test behavior, not implementation
- **Integration tests over unit tests** - Does the whole thing work?

## PROJECT-SPECIFIC BUILD
- **Single build command** - `xmake` should be enough
- **No build scripts** - If you need a script, the build system is wrong
- **Static linking preferred** - Fewer runtime dependencies
- **No configuration files** - Use command line arguments

## PROJECT-SPECIFIC RUNTIME OPTIMIZATION
- **Cache at startup** - Environment variables, config paths, process PIDs
- **Minimize system calls** - Avoid repeated `getenv()`, `pidof`, file operations
- **Direct function calls** - No indirection overhead during runtime
- **Simple control flow** - Linear execution paths, minimal branching
- **Fail fast on errors** - Don't waste cycles on recovery attempts

## PROJECT-SPECIFIC VALIDATION
- **Validate inputs that matter** - Don't validate everything "just in case"
- **Examples of what matters**: File paths, process IDs, user input
- **Examples of what doesn't**: Integer overflow for mouse coordinates, bounds checking for unlikely values

## PROJECT-SPECIFIC DEPENDENCIES
- **Current dependencies**: fmt, jsoncpp
- **Dependency limit**: < 3 (currently 2)
- **Scale to project size**: 1-3 for simple tools, 5-10 for complex apps
- **Question each dependency**: Do you really need it?
- **No optional dependencies** - If it's optional, it's probably unnecessary

## PROJECT-SPECIFIC CONSTANTS
- **DEFAULT_BAR_THRESHOLD**: 50 pixels
- **MOUSE_ACTIVATION_ZONE**: 7 pixels from top of monitor
- **POLLING_INTERVAL**: 80ms mouse position polling frequency
- **MIN_THRESHOLD**: 1 pixel
- **MAX_THRESHOLD**: 1000 pixels
- **LOOP_TIMEOUT**: 30s maximum time in any single loop iteration
- **MAX_LOOP_ITERATIONS**: 1000 iterations before timeout check

## PROJECT-SPECIFIC FINAL RULE
**If you're not sure whether to add something, don't add it.**
**If you're not sure whether to remove something, remove it.**
**When in doubt, choose the simpler option.**

Remember: This is a tool that hides waybar. It's not a distributed system, it's not a web service, it's not enterprise software. Keep it simple, keep it fast, keep it working.

## AUDITOR INSTRUCTIONS REFERENCE
**For comprehensive auditor guidance, see `.cursorrules` - AUDITOR INSTRUCTIONS section.**
**The auditor instructions include:**
- **Real violations vs flavor differences** - Clear criteria for what to fix
- **Contextual decision framework** - 5-step process for evaluating violations
- **Project-specific context** - Waybar tool constraints and requirements
- **Conflict resolution hierarchy** - How to handle rule conflicts
- **Edge case guidance** - When to accept apparent violations

## DEVELOPER GUIDELINES REFERENCE
**For developer guidance on implementing anti-kruft principles, see `.cursorrules` - DEVELOPER GUIDELINES section.**
**The developer guidelines include:**
- **Decision framework** - 5-step process for evaluating auditor feedback
- **Developer responsibilities** - When to challenge vs accept auditor suggestions
- **Common auditor mistakes** - Suggestions that violate anti-kruft principles
- **Acceptance criteria** - When auditor feedback should be implemented
- **Examples** - Specific cases of what to reject vs accept
