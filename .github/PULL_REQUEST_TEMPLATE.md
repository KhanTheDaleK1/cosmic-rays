# Pull Request Template

## Description
Please include a summary of the changes and which issue is fixed. Include relevant motivation and context.

Fixes # (issue number)

## Type of Change
- [ ] 🧪 **DSP / Algorithm:** New granular logic, filters, or modulation.
- [ ] 🎨 **UI / UX:** Visual updates, skinning, or layout changes.
- [ ] 🐛 **Bug Fix:** Resolving a specific issue or glitch.
- [ ] 🚀 **CI / CD:** Build system, versioning, or deployment updates.
- [ ] 📚 **Documentation:** README, CONTRIBUTING, or SECURITY updates.

## Critical DSP Checklist (The "Golden Rules")
If this change affects the audio processing path (`processBlock`):
- [ ] No memory allocations (`new`, `malloc`) in the audio thread.
- [ ] No mutex locks or blocking calls.
- [ ] No file I/O or system calls.
- [ ] Verified that mathematical precision (float vs double) is appropriate.

## General Quality Checklist
- [ ] My code follows the style guidelines of this project (CamelCase for types, camelCase for variables).
- [ ] I have performed a self-review of my own code.
- [ ] I have commented my code, particularly in hard-to-understand areas.
- [ ] I have run the `python3 generate_version.py` script to ensure versioning is in sync.
- [ ] I have tested the changes on at least one native OS (macOS, Windows, or Linux).

## Screenshots / Audio Clips (If applicable)
*Attach images of the new UI or recordings of the new DSP behavior if possible.*

---
*“In the granular void, every grain counts.”*
