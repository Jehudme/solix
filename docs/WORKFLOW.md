# Solix Engineering Workflow & Development Lifecycle

Each phase and feature in Solix must strictly adhere to the following development lifecycle:

1. **Dedicated Branching**: Create a feature branch:
   ```bash
   git checkout -b phase-N-descriptive-name
   ```
2. **Implementation**: Implement code changes according to the detailed architectural and solution specification.
3. **Commit Implementation**:
   ```bash
   git commit -m "feat/fix: <description>"
   ```
4. **Test Specification Analysis & Update**:
   - Before implementing test code, analyze the codebase and recent changes to determine required test coverage.
   - Update `tests/statements/TESTS.md` by adding or updating the test scenarios to be implemented.
   - By default, all newly added test cases in `tests/statements/TESTS.md` must be tagged with `[NOT IMPLEMENTED]`.
5. **Commit Test Specification**:
   ```bash
   git commit -m "test(spec): update test specifications in TESTS.md"
   ```
6. **Unit & Statement Test Implementation**:
   - Implement or update Catch2 unit tests in `tests/statements/` organized cleanly by construct category (`modules/`, `declarations/`, `control_flow/`, `expressions/`).
   - As test cases are implemented and verified, update their tags in `tests/statements/TESTS.md` from `[NOT IMPLEMENTED]` to `[IMPLEMENTED]`.
7. **Commit Tests**:
   - Commit tests incrementally between statements:
     ```bash
     git commit -m "test: implement Catch2 test suite for <construct>"
     ```
8. **Full Test Suite Run**:
   - Run the complete test suite:
     ```bash
     ctest --test-dir build --output-on-failure
     ```
9. **Merge**:
   - Merge back into `master` using non-fast-forward merge:
     ```bash
     git checkout master && git merge --no-ff phase-N-descriptive-name
     ```
