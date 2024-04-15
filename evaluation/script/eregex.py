import re

class ERegex:
    def __init__(self, pattern: str) -> None:
        if len(pattern) == 0:
            self.pattern = re.compile(pattern)
            self.negative = False
            return
        if pattern[0] == '!':
            if len(pattern) == 1: # '!'
                raise ValueError('bad eregex format: "!"')
            if pattern[1] != '!': # '!xxx'
                self.pattern = re.compile(pattern[1:])
                self.negative = True
                return
            if len(pattern) == 2: # '!!'
                self.pattern = re.compile('!')
                self.negative = False
                return
            if pattern[2] == '!': # '!!!xxx'
                self.pattern = re.compile(pattern[2:])
                self.negative = True
                return
            else: # '!!xxx'
                self.pattern = re.compile(pattern[1:])
                self.negative = False
                return
        self.pattern = re.compile(pattern)
        self.negative = False

    def match(self, string: str) -> bool:
        from operator import xor
        return xor(re.search(self.pattern, string) is not None, self.negative)

