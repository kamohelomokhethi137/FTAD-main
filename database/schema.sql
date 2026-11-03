
PRAGMA foreign_keys = ON;

-- Financial transactions
CREATE TABLE IF NOT EXISTS transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    transaction_id TEXT NOT NULL UNIQUE,
    timestamp TEXT NOT NULL,
    sender_id TEXT NOT NULL,
    receiver_id TEXT NOT NULL,
    transaction_type TEXT NOT NULL,
    amount REAL NOT NULL CHECK (amount >= 0),
    currency TEXT NOT NULL DEFAULT 'LSL',
    status TEXT NOT NULL DEFAULT 'completed',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- Detected anomalies
CREATE TABLE IF NOT EXISTS anomalies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    transaction_id INTEGER NOT NULL,
    detection_method TEXT NOT NULL,
    risk_score REAL NOT NULL
        CHECK (risk_score >= 0 AND risk_score <= 100),
    risk_level TEXT NOT NULL
        CHECK (risk_level IN ('low', 'medium', 'high')),
    reason TEXT NOT NULL,
    detected_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (transaction_id)
        REFERENCES transactions(id)
        ON DELETE CASCADE
);

-- Investigation cases
CREATE TABLE IF NOT EXISTS investigations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    anomaly_id INTEGER NOT NULL,
    case_status TEXT NOT NULL DEFAULT 'pending'
        CHECK (case_status IN (
            'pending',
            'in_progress',
            'resolved',
            'dismissed'
        )),
    assigned_to TEXT,
    notes TEXT,
    opened_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    closed_at TEXT,

    FOREIGN KEY (anomaly_id)
        REFERENCES anomalies(id)
        ON DELETE CASCADE
);

-- Review history for investigation cases
CREATE TABLE IF NOT EXISTS investigation_events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    investigation_id INTEGER NOT NULL,
    event_type TEXT NOT NULL,
    previous_status TEXT,
    new_status TEXT,
    notes TEXT,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (investigation_id)
        REFERENCES investigations(id)
        ON DELETE CASCADE
);

-- Indexes for faster searches
CREATE INDEX IF NOT EXISTS idx_transactions_timestamp
ON transactions(timestamp);

CREATE INDEX IF NOT EXISTS idx_transactions_sender
ON transactions(sender_id);

CREATE INDEX IF NOT EXISTS idx_anomalies_risk
ON anomalies(risk_level);

CREATE INDEX IF NOT EXISTS idx_anomalies_transaction
ON anomalies(transaction_id);

CREATE INDEX IF NOT EXISTS idx_investigations_status
ON investigations(case_status);

CREATE INDEX IF NOT EXISTS idx_investigations_anomaly
ON investigations(anomaly_id);

CREATE INDEX IF NOT EXISTS idx_investigation_events_case
ON investigation_events(investigation_id);
