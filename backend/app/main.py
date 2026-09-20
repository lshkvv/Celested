from fastapi import FastAPI

app = FastAPI(title="Celested API")


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}
