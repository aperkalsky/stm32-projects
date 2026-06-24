# generic result wrapper

from dataclasses import dataclass

from protocol.status import TlvStatus


@dataclass
class CommandResult:

    status: TlvStatus
    data: object | None