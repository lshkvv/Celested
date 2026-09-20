# Celested

Celested is a desktop-first astrophotography assistant for amateur astronomers. It lets users upload telescope images, run sky-field analysis, identify likely astronomical objects in the frame, read grounded descriptions, and ask follow-up questions about what appears in the image.

## Status

This repository is structured for a free MVP.

Current stack:

- **Desktop:** PySide6 (Qt for Python)
- **Backend:** FastAPI
- **Queue:** RQ + Redis
- **Database:** SQLite in WAL mode for the first iteration
- **Auth:** Custom email/password auth with Argon2id password hashing and JWT tokens
- **LLM:** OpenRouter via OpenAI-compatible API, using free models or `openrouter/free`

## Goals

Celested focuses on a practical and explainable astrophotography workflow:

1. Upload a telescope image.
2. Run asynchronous analysis.
3. Solve the field and recover sky calibration metadata.
4. Match catalog objects inside the frame.
5. Present object summaries and annotations.
6. Answer user questions using grounded image context.

The MVP intentionally prioritizes a deterministic solve-and-match pipeline over speculative end-to-end image guessing. This keeps the product more transparent and more reliable for real amateur astrophotography data.

## Architecture

The project is split into three runtime components:

- `desktop/` — the PySide6 desktop client
- `backend/` — the FastAPI API and orchestration layer
- `worker/` — RQ workers for long-running background jobs

Supporting folders:

- `shared/` — shared prompts, DTOs, constants
- `storage/` — local uploads and derived assets during development
- `tests/` — automated tests

## Repository Layout

```text
celested/
├── README.md
├── .env.example
├── .gitignore
├── docker-compose.yml
├── desktop/
├── backend/
├── worker/
├── shared/
├── storage/
└── tests/
```

## Tech Choices

### Why PySide6

PySide6 is the official Qt for Python binding and is a strong fit for a desktop-first product that needs a rich UI, custom image rendering, dockable panels, and stable cross-platform support.

### Why RQ and Redis

RQ is a simple Redis-backed Python job queue. It is well suited to asynchronous image-analysis workflows and already provides queueing, job IDs, lifecycle states, and worker processes without the operational weight of a more complex distributed system.

### Why SQLite first

SQLite keeps the MVP lightweight and easy to run locally. In WAL mode, readers and a writer can operate concurrently, although SQLite still allows only one writer at a time, which is acceptable for a small first version.

### Why custom auth

A custom auth layer keeps the MVP free from paid identity services and external coupling. Passwords should be hashed with Argon2id, which is broadly recommended for new applications, including in OWASP guidance.

### Why OpenRouter

OpenRouter offers an OpenAI-compatible API and a free routing option, which makes it practical for a zero-cost MVP while preserving the option to switch models later without changing the whole application.

## Core MVP Flow

1. The user signs up or signs in.
2. The desktop app uploads an astrophotography image.
3. The backend stores metadata and enqueues an analysis job.
4. An RQ worker performs the analysis pipeline.
5. The backend exposes job status and detected-object results.
6. The desktop UI shows matched objects and generated summaries.
7. The user asks a question about the image, and the backend calls OpenRouter with grounded context.

## Planned Analysis Pipeline

The initial pipeline should remain modular even if some steps are implemented inside one worker function at the start.

1. Validate the input file.
2. Generate a preview and normalized derivative.
3. Run plate solving.
4. Store calibration metadata.
5. Match catalog objects in the field of view.
6. Compute annotation positions.
7. Generate concise educational summaries.
8. Save results and mark the job as complete.

## Authentication Model

The MVP auth system should include:

- Email/password registration
- Password hashing with Argon2id
- JWT access tokens
- Optional refresh tokens
- Protected API routes

The implementation should rely on standard libraries rather than custom cryptography.

## Environment Variables

Create a `.env` file based on `.env.example`.

```env
APP_ENV=development
BACKEND_HOST=127.0.0.1
BACKEND_PORT=8000
DESKTOP_API_BASE_URL=http://127.0.0.1:8000
DATABASE_URL=sqlite:///./celested.db
SQLITE_WAL=true
JWT_SECRET_KEY=change-me
JWT_ALGORITHM=HS256
JWT_ACCESS_TOKEN_EXPIRE_MINUTES=30
JWT_REFRESH_TOKEN_EXPIRE_DAYS=7
REDIS_URL=redis://127.0.0.1:6379/0
RQ_DEFAULT_QUEUE=default
RQ_ANALYSIS_QUEUE=analysis
OPENROUTER_API_KEY=
OPENROUTER_MODEL=openrouter/free
OPENROUTER_BASE_URL=[https://openrouter.ai/api/v1](https://openrouter.ai/api/v1)
STORAGE_ROOT=./storage
```

## Local Development

### 1. Start Redis

Using Docker:

```bash
docker run --name celested-redis -p 6379:6379 redis:7
```

### 2. Install backend dependencies

```bash
cd backend
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### 3. Install worker dependencies

```bash
cd ../worker
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### 4. Install desktop dependencies

```bash
cd ../desktop
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### 5. Run the backend

```bash
cd backend
uvicorn app.main:app --reload --host 127.0.0.1 --port 8000
```

### 6. Run the worker

```bash
cd worker
rq worker analysis default
```

### 7. Run the desktop app

```bash
cd desktop
python -m app.main
```

## Suggested First Milestones

### Milestone 1

- Project scaffolding
- SQLite models
- Auth endpoints
- Desktop login screen
- Image upload endpoint

### Milestone 2

- Redis and RQ wiring
- Analysis job records
- Worker polling in desktop UI
- Basic image details page

### Milestone 3

- Catalog object storage
- Object list and object details panel
- Generated per-object summaries
- Grounded chat endpoint

### Milestone 4

- Better overlay rendering
- Retry and failure handling
- Caching and cleanup jobs
- Packaging for desktop distribution

## Security Notes

- Never store plaintext passwords.
- Use Argon2id through a maintained library such as `argon2-cffi`.
- Keep JWT secrets out of version control.
- Restrict the chat layer to grounded image context.
- Validate uploaded file types and size limits.

## Near-Term Roadmap

- Plate-solving integration abstraction
- Local object catalogs
- Object annotation overlay
- Better chat guardrails
- FITS-first ingestion improvements
- Cross-platform packaging

## Long-Term Ideas

- Unknown-object classification
- Better astrophotography metadata import
- Observation session management
- Object recommendations near the current field
- Collaboration and shared image libraries

## License

Add a license before the first public release.
