# generic result wrapper

# class representing the device response in the form: status + data

from dataclasses import dataclass
from protocol.status import TlvStatus

@dataclass
class CommandResult:

    status: TlvStatus
    data: object | None