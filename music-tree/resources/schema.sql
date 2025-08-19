-- Artists table
CREATE TABLE IF NOT EXISTS artists (
    id TEXT PRIMARY KEY,      -- Discogs artist ID (string)
    name TEXT NOT NULL,
    profile TEXT,             -- biography/description
    data_quality TEXT,        -- Discogs field
    resource_url TEXT
);

CREATE TABLE Members (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    artist_id INTEGER NOT NULL,   -- The band/group artist ID
    member_id INTEGER,            -- Discogs ID of the member
    name TEXT NOT NULL,
    active BOOLEAN,
    resource_url TEXT,
    FOREIGN KEY (artist_id) REFERENCES artists(id) ON DELETE CASCADE
);


-- Releases table
CREATE TABLE IF NOT EXISTS releases (
    id TEXT PRIMARY KEY,      -- Discogs release ID (string)
    title TEXT NOT NULL,
    year INTEGER,
    country TEXT,
    genre TEXT,
    style TEXT,
    resource_url TEXT,
    data_quality TEXT
);

-- Junction table: Many-to-many between releases and artists
CREATE TABLE IF NOT EXISTS release_artists (
    release_id TEXT NOT NULL,
    artist_id TEXT NOT NULL,
    role TEXT,   -- e.g. "Main", "Featuring", "Producer", etc.
    PRIMARY KEY (release_id, artist_id),
    FOREIGN KEY (release_id) REFERENCES releases(id) ON DELETE CASCADE,
    FOREIGN KEY (artist_id) REFERENCES artists(id) ON DELETE CASCADE
);

-- Tracks (optional, in case we want per-track detail)
CREATE TABLE IF NOT EXISTS tracks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    release_id TEXT NOT NULL,
    position TEXT,      -- e.g. "A1", "1", "B2"
    title TEXT NOT NULL,
    duration TEXT,
    FOREIGN KEY (release_id) REFERENCES releases(id) ON DELETE CASCADE
);

-- Labels (optional, Discogs often lists them)
CREATE TABLE IF NOT EXISTS labels (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS release_labels (
    release_id TEXT NOT NULL,
    label_id TEXT NOT NULL,
    catalog_number TEXT,
    PRIMARY KEY (release_id, label_id),
    FOREIGN KEY (release_id) REFERENCES releases(id) ON DELETE CASCADE,
    FOREIGN KEY (label_id) REFERENCES labels(id) ON DELETE CASCADE
);

-- Indexes for fast joins
CREATE INDEX IF NOT EXISTS idx_release_artists_artist ON release_artists(artist_id);
CREATE INDEX IF NOT EXISTS idx_release_artists_release ON release_artists(release_id);
