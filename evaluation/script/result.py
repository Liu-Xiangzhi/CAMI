from pydantic import BaseModel

class Case(BaseModel):
    name: str
    output: list[tuple[str,str,str]] # cmd name, stdout, stderr
    alarmed : bool

class Tool(BaseModel):
    positive: list[Case]
    negative: list[Case]
