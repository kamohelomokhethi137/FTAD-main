FTAD --- Project Development To-Do
Project: Financial Transaction Anomaly Detector
Stack: C++17, Qt 6, SQLite, CMake/Ninja, GoogleTest; Python for
offline ML training
Target: Linux desktop (initial development on Windows)
Phase 0 --- Environment and prototype
- [x] Install and verify G++, CMake, Qt 6, SQLite, GoogleTest, GDB and
  Ninja
- [x] Configure VS Code, CMake and MSYS2 UCRT64
- [x] Build and launch the first Qt window
- [x] Create a dashboard mockup with sidebar and summary cards


Phase 1 --- Modular frontend and working navigation (START HERE)
- [ ] Create frontend, backend, ml, database, data, tests,
  config and docs folders as planned
- [ ] Move the dashboard from main.cpp into
  frontend/include/MainWindow.h and frontend/src/MainWindow.cpp
- [ ] Keep main.cpp as a minimal application entry point
- [ ] Create separate Qt page classes: Dashboard, Transactions,
  Detection, Investigations, Reports, Settings
- [ ] Use QStackedWidget to switch pages when sidebar buttons are
  clicked
- [ ] Highlight active navigation and display correct page headings
- [ ] Move styles into frontend/resources/styles/theme.qss
- [ ] Update CMakeLists.txt for the modular sources and resources
- [ ] Build and manually test all navigation buttons
Done when: All six pages open correctly and the app still compiles.



Phase 2 --- Database and transaction model
- [ ] Define transaction fields, types, timestamps and account
  identifiers
- [ ] Create database/schema.sql for transactions, detection
  results, investigations and settings
- [ ] Implement database access in the C++ backend using SQLite
- [ ] Add database initialization, safe queries and error handling
- [ ] Verify saving and retrieving records; add integration tests
Done when: Transaction records persist between app launches.



Phase 3 --- Transaction management
- [ ] Build a transaction table with search, filters and details
- [ ] Add manual transaction entry with validation
- [ ] Import CSV files with column mapping and error reporting
- [ ] Detect duplicates and invalid/missing values during import
- [ ] Save imported transactions and update dashboard statistics
- [ ] Test import, validation and transaction operations
Done when: Users can import, add and browse transactions.



Phase 4 --- Data cleaning and machine-learning training
- [ ] Download PaySim from Kaggle; keep original CSV in data/raw/
- [ ] Inspect columns, missing values, duplicates, class imbalance and
  data leakage
- [ ] Write repeatable cleaning scripts in ml/preprocessing/
- [ ] Create a manageable representative sample for an 8 GB laptop
- [ ] Split training/validation/test data without leakage
- [ ] Establish a rule/statistical baseline and choose an ML model
- [ ] Train candidate model(s) in ml/training/
- [ ] Evaluate precision, recall, F1, false positives and performance
- [ ] Document model limitations and export a C++-compatible inference
  artifact
Done when: The model and preprocessing can be reproduced and
evaluated.



Phase 5 --- Automatic anomaly detection
- [ ] Implement AnomalyDetector, BehaviorAnalyzer and RiskScorer
- [ ] Integrate model inference into the C++ backend
- [ ] Analyze transactions automatically after import or entry
- [ ] Add a re-analyze action when settings/model change
- [ ] Save risk scores, reasons, timestamps and model version
- [ ] Show flagged transactions on the Detection page
- [ ] Test normal, suspicious, missing-data and edge cases
Done when: New transactions receive reproducible risk assessments.



Phase 6 --- Investigations and reports
- [ ] Show flagged transactions as investigation cases
- [ ] Add case statuses, reviewer notes and history
- [ ] Save investigation changes to SQLite
- [ ] Generate summaries by date, risk and investigation outcome
- [ ] Export reports to CSV and/or PDF
- [ ] Connect dashboard statistics and recent activity to real records
- [ ] Test investigation and reporting workflows
Done when: Users can investigate and export findings.



Phase 7 --- Settings and usability
- [ ] Add threshold, analysis and model settings
- [ ] Validate and persist configuration
- [ ] Add progress feedback and clear errors
- [ ] Keep the UI responsive during large imports and analysis
- [ ] Review input validation, logging and database reliability



Phase 8 --- Testing and Linux verification
- [ ] Run GoogleTest unit, integration and regression tests via CTest
- [ ] Run performance tests and collect test evidence/coverage
- [ ] Set up Ubuntu/WSL or another Linux environment
- [ ] Install Linux equivalents of required dependencies
- [ ] Recompile C++ source as a native Linux executable
- [ ] Run GDB and Valgrind checks on Linux
- [ ] Verify Qt, SQLite, CSV and ML inference on Linux
- [ ] Fix portability issues and rerun regression tests
Done when: FTAD runs and passes documented tests on Linux.



Phase 9 --- Documentation and submission
- [ ] Complete README.md with build/run instructions
- [ ] Document architecture, data flow, database schema and ML
  decisions
- [ ] Prepare requirements, test plan, cases and results
- [ ] Record limitations, screenshots and demonstration steps
- [ ] Check assignment submission requirements and package source code