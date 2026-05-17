PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS images (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT NOT NULL UNIQUE,
    title TEXT,
    created_at TEXT,
    center_ra REAL,
    center_dec REAL,
    field_radius_deg REAL,
    analysis_status TEXT,
    analyzed_at TEXT
);

CREATE TABLE IF NOT EXISTS object_types (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    description TEXT
);

CREATE TABLE IF NOT EXISTS objects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT,
    type_id INTEGER,
    image_id INTEGER NOT NULL,
    ra TEXT,
    dec TEXT,
    magnitude TEXT,
    constellation TEXT,
    messier TEXT,
    ngc TEXT,
    ic TEXT,
    identification_status TEXT,
    guessed_type TEXT,
    simbad_type TEXT,
    FOREIGN KEY(type_id) REFERENCES object_types(id) ON DELETE SET NULL,
    FOREIGN KEY(image_id) REFERENCES images(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS detections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    image_id INTEGER NOT NULL,
    object_id INTEGER,
    x REAL NOT NULL,
    y REAL NOT NULL,
    width REAL NOT NULL,
    height REAL NOT NULL,
    confidence REAL DEFAULT 1.0,
    FOREIGN KEY(image_id) REFERENCES images(id) ON DELETE CASCADE,
    FOREIGN KEY(object_id) REFERENCES objects(id) ON DELETE SET NULL
);
