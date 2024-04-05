from pydantic import BaseModel

class Command(BaseModel):
    command: str
    interrupt_on_error: bool

class Tool(BaseModel):
    description: str
    commands: list[Command]
    detect_word: str

class Filter(BaseModel):
    tool : str
    rule : list[str]

class Config(BaseModel):
    tools: dict[str, Tool]
    filter: list[Filter]