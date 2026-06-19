# Contributing to Smart Campus Parking System

Welcome to the team. Please read and follow these guidelines so we can collaborate efficiently across hardware, firmware, and software modules.

---

## 1. Cloning the Repository

```bash
git clone https://github.com/<org>/smart-campus-parking-system.git
cd smart-campus-parking-system
```

Install Python dependencies after cloning:

```bash
pip install -r python/requirements.txt
```

---

## 2. Branch Naming

Always work on a feature branch — **never commit directly to `main`**.

| Type | Pattern | Example |
|------|---------|---------|
| Feature | `feature/<short-description>` | `feature/sensor-uart-parser` |
| Bug fix | `fix/<short-description>` | `fix/gate-servo-timing` |
| Documentation | `docs/<short-description>` | `docs/wiring-diagram-update` |
| FPGA / RTL | `fpga/<short-description>` | `fpga/slot-counter-module` |
| Arduino | `arduino/<short-description>` | `arduino/sensor-debounce` |
| Integration | `integration/<short-description>` | `integration/e2e-test-script` |

```bash
# Create and switch to a new branch
git checkout -b feature/my-feature
```

---

## 3. Commit Messages

Follow the **Conventional Commits** format:

```
<type>(<scope>): <short summary>

[optional body — explain WHY, not what]
```

### Types
| Type | Use for |
|------|---------|
| `feat` | New feature or capability |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `test` | Adding or updating tests/testbenches |
| `refactor` | Code restructure with no behavior change |
| `chore` | Build scripts, CI, tooling changes |
| `hw` | Hardware-specific changes (pinouts, constraints) |

### Scopes
`fpga`, `arduino`, `python`, `integration`, `docs`

### Examples
```
feat(fpga): add slot occupancy state machine for 20 slots

fix(arduino): debounce ultrasonic sensor reads to prevent false triggers

docs(docs): add wiring diagram for sensor module v2

test(fpga): add testbench for UART TX module
```

---

## 4. Pull Request Process

1. **Push your branch** to GitHub:
   ```bash
   git push -u origin feature/my-feature
   ```

2. **Open a Pull Request** on GitHub targeting the `main` branch.

3. **Fill in the PR template:**
   - What does this PR do?
   - How was it tested? (simulation results, serial output, photos of hardware if relevant)
   - Any dependencies or prerequisites?

4. **Request at least one reviewer** from the team — ideally someone who owns an adjacent module.

5. **Address all review comments** before merging. Use "resolve conversation" only after fixing the issue.

6. **Merge strategy:** Use **Squash and Merge** for small feature branches to keep `main` history clean. Use **Merge Commit** for large integration PRs so individual commits are preserved.

7. **Delete the branch** after merging (GitHub can do this automatically).

---

## 5. Code Review Expectations

- Reviews should be completed within **48 hours** of being assigned.
- Be constructive and specific — reference line numbers and suggest alternatives.
- Approving a PR means you have read the diff and are confident it won't break your module.

---

## 6. Hardware Change Protocol

Before merging any change that affects hardware interfaces (pin assignments, baud rates, message formats):

1. Update the relevant `README.md` interface section.
2. Notify the team in the group chat.
3. Tag the PR with the `hardware-change` label.

---

## 7. Questions?

Open a GitHub Issue with the `question` label, or ping the team in the group chat.
