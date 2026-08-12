#!/usr/bin/env python3

import time
import logging
import struct
import pytest
from enum import Enum
from typing import Callable, Final, List, Union, Literal
from typing_extensions import Protocol, Self
from random import getrandbits

from dataclasses import dataclass, field
from typing import Any, ClassVar, Type

LOGGER = logging.getLogger("IPC")

IPC_MAX_MESSAGE_SIZE_WORDS = 63

class RegisterProtocol(Protocol):
    address: int


class FieldProtocol(Protocol):
    address: int
    offset: int
    mask: int


class ReadOnlyError(Exception):
    pass


class RegisterReset:
    """Descriptor class for a register reset property"""

    def __get__(self, _, obj_type: Type["MemRegister"]) -> int:
        """
        Compute the reset value of the register as being
        the concatenation of its fields' reset values
        """
        return sum(
            attribute.reset << attribute.offset
            for attribute in obj_type.__dict__.values()
            if isinstance(attribute, MemField)
        )


class ReadOnlyMeta(type):
    """Metaclass to ensure attributes cannot be written"""

    def __setattr__(self, name: str, value: Any) -> None:
        raise ReadOnlyError(
            f"{self.__qualname__}: cannot set "
            f"{name} to {value=:#x}: attribute is read-only"
        )

    def __repr__(self) -> str:
        return self.__qualname__


@dataclass(frozen=True, repr=False)
class MemField:
    """Field class"""

    address: int
    offset: int
    width: int
    reset: int
    mask: int = field(init=False, repr=False)

    def __post_init__(self) -> None:
        object.__setattr__(self, "mask", (2**self.width - 1) << self.offset)

    def __repr__(self) -> str:
        return (
            f"{self.__class__.__name__}("
            f"address={self.address:#010x}, "
            f"offset={self.offset}, "
            f"width={self.width}, "
            f"reset={self.reset:#x})"
        )


class MemRegister(metaclass=ReadOnlyMeta):
    """Register class"""

    address: ClassVar[int]
    reset: ClassVar[int] = RegisterReset()  # type: ignore


class MemRegion(metaclass=ReadOnlyMeta):
    """Region class"""

    address: ClassVar[int]


class IPC_REGS(MemRegion):
    address = 0x200000

    class MSG_ID_1(MemRegister):
        address = 0x200000
        CPU_BUSY = MemField(0x200000, 0, 1, 0x0)
        CPU_DONE = MemField(0x200000, 1, 1, 0x0)
        MSG_SIZE = MemField(0x200000, 2, 8, 0x0)
        MSG_TYPE = MemField(0x200000, 10, 4, 0x0)
        MSG_CMDS = MemField(0x200000, 14, 4, 0x0)

    class MSG_ID_2(MemRegister):
        address = 0x200004
        PARENT_BUSY = MemField(0x200004, 0, 1, 0x0)
        PARENT_DONE = MemField(0x200004, 1, 1, 0x0)
        MSG_SIZE = MemField(0x200004, 2, 8, 0x0)
        MSG_TYPE = MemField(0x200004, 10, 4, 0x0)
        MSG_CMDS = MemField(0x200004, 14, 4, 0x0)

    class CPU_TO_PARENT_0(MemRegister):
        address = 0x200008
        CPU_TO_PARENT_DATA = MemField(0x200008, 0, 32, 0x0)

    class CPU_TO_PARENT_63(MemRegister):
        address = 0x200104
        CPU_TO_PARENT_DATA = MemField(0x200104, 0, 32, 0x0)

    class PARENT_TO_CPU_0(MemRegister):
        address = 0x200108
        PARENT_TO_CPU_DATA = MemField(0x200108, 0, 32, 0x0)

    class PARENT_TO_CPU_63(MemRegister):
        address = 0x200204
        PARENT_TO_CPU_DATA = MemField(0x200168, 0, 32, 0x0)


class IPCException(Exception):
    def __init__(self, message: str):
        self.message = message
        super().__init__(self.message)


class InterfaceProtocol(Protocol):
    """The interfaces in the TVE comply with this interface protocol."""

    def tpdi_read(self, address: int, length: int = 1) -> List[int]:
        ...

    def tpdi_write(self, address: int, data: Union[List[int], int]):
        ...

class IPC_CMDS(int, Enum):
    # 4 bit command field
    NONE = 0
    FAIL = 2
    PASS = 3
    GETRND = 4
    GETDATA = 5
    BULK_TRANSFER = 6

    def __int__(self):
        return self.value


class IPC_MSG_TYPES(int, Enum):
    # 4 bit type field
    GEN = 1
    STR = 2

    def __int__(self):
        return self.value


class IPC:

    DEFAULT_POLLING_TIMEOUT: Final[int] = 10  # seconds
    DEFAULT_POLLING_PERIOD: Final[float] = 0.1  # seconds

    LogData: bool

    DataLogs: List[List[int]] = []
    """List of all generic data (integers and arrays) sent from child process.
       Only accessible when test passes."""

    MessageLogs: List[str] = []
    """List of all strings sent from child process.
       Only accessible when test passes."""

    def __init__(
        self,
        read_function: Callable[[int, int], List[int]],
        write_function: Callable[[int, Union[List[int], int]], None],
        polling_timeout: int = DEFAULT_POLLING_TIMEOUT,
        polling_period: float = DEFAULT_POLLING_PERIOD,
        log_data: bool = True,
    ) -> None:
        """Create a new IPC instance

        Args:
            read_function (Callable[[int, int], List[int]]):
                function used to read data.
            write_function (Callable[[int, Union[List[int], int]], None]):
                function used to write data.
        """

        self.read_function = read_function
        self.write_function = write_function

        self.polling_timeout = polling_timeout
        self.polling_period = polling_period

        self.LogData = log_data

        # initialize register
        self.__flush_status_register()

    def __flush_status_register(self):

        self.write_function(IPC_REGS.MSG_ID_2.address, 0)

    def main_process(self):
        """Starts a new communication session. Session will print all messages,
        interpret all commands and timeout if no message sent from child process.
        """

        self.__poll_status_register(method="session")

    def wait(self) -> Union[List[int], str, None]:
        """Waits for one message at a time from child process."""
        return self.__poll_status_register(method="wait")

    @classmethod
    def from_interface(
        cls,
        interface: InterfaceProtocol,
        timeout: int = DEFAULT_POLLING_TIMEOUT,
        polling_period: float = DEFAULT_POLLING_PERIOD,
        log_data: bool = True,
    ) -> Self:
        """Create a new IPC instance from an interface

        Args:
            interface (InterfaceProtocol): the interface used to communicate.

        Returns:
            the new IPC instance.
        """
        return cls(
            read_function=interface.tpdi_read,
            write_function=interface.tpdi_write,
            polling_timeout=timeout,
            polling_period=polling_period,
            log_data=log_data,
        )

    def __read_buffer(self, size: int, type: IPC_MSG_TYPES) -> Union[List[int], str]:
        """Read buffer contents, called for every new message

        Args:
            size (int)   : size of message.
            type (int)   : type of message (string or generic data).

        Returns: data (List[int]) or message (str) depending on type of message.
        """
        if size <= 0:
            return

        __to_four_bytes = struct.Struct("<I").pack

        buffer_address = IPC_REGS.CPU_TO_PARENT_0.address

        n_registers = size // 4 + (size % 4 > 0)

        read_data = self.read_function(buffer_address, n_registers)

        if type == IPC_MSG_TYPES.GEN:
            if self.LogData:
                self.DataLogs.append(read_data)
            return read_data

        elif type == IPC_MSG_TYPES.STR:
            message = ""
            for word in read_data:
                b1, b2, b3, b4 = __to_four_bytes(word)
                message += chr(b1) + chr(b2) + chr(b3) + chr(b4)
            message = message[:size] # strip bytes of last word not part of the message
            if self.LogData:
                self.MessageLogs.append(message)
            return message

    def __execute(self, cmd: IPC_CMDS) -> None:
        """Execute commands like test_passed, get_random_number.

        Args:
            cmd (int): command to execute (IPC_CMDS)
        """

        if cmd == IPC_CMDS.FAIL:
            raise IPCException(f"TEST FAILED")

        elif cmd == IPC_CMDS.GETRND:
            self.__send_random_number()

        elif cmd == IPC_CMDS.GETDATA:
            return

    def send(self, data: Union[int, List[int]]) -> None:
        """Send data to child process. Blocks until child process finishes reading data.

        Args: data (int or List[int])
        """

        # wait till child requests data
        current_time = time.monotonic()
        timeout_time = current_time + self.polling_timeout

        while current_time < timeout_time:

            status_register_data = self.__read(IPC_REGS.MSG_ID_1)

            if (
                done_flag := (status_register_data & IPC_REGS.MSG_ID_1.CPU_DONE.mask)
                >> IPC_REGS.MSG_ID_1.CPU_DONE.offset
            ):
                cmd = (
                    status_register_data & IPC_REGS.MSG_ID_1.MSG_CMDS.mask
                ) >> IPC_REGS.MSG_ID_1.MSG_CMDS.offset

                if cmd == IPC_CMDS.GETDATA:

                    # unset cpu done flag -> indicate parent has read get_data request
                    self.__write(IPC_REGS.MSG_ID_1.CPU_DONE, 0)

                    self.__flush_status_register()

                    if isinstance(data, int):
                        data = [data]

                    if len(data) > IPC_MAX_MESSAGE_SIZE_WORDS:
                        LOGGER.warning(f"IPC max message size is {IPC_MAX_MESSAGE_SIZE_WORDS}, message truncated.")
                        data = data[:IPC_MAX_MESSAGE_SIZE_WORDS]

                    LOGGER.debug(f"Sending {data}")

                    buffer_address = IPC_REGS.PARENT_TO_CPU_0.address
                    for i in data:
                        self.write_function(buffer_address, i)
                        buffer_address += 4

                    # set parent done flag -> indicate cpu can start reading
                    self.__write(IPC_REGS.MSG_ID_2.PARENT_DONE, 1)

                    # wait for parent done unset -> indicates cpu has finished reading 
                    self.__wait_for_parent_done_unset(timeout_time)
                    return

            time.sleep(self.polling_period)
            current_time = time.monotonic()

        raise IPCException(
            f"Send data function called but target did not expect data for {self.polling_timeout} s"
        )

    def __wait_for_parent_done_unset(self, timeout_time: float):
        """Parent done flag will only be reset by child process. Unsetting this bit indicates child has completed processing."""

        while time.monotonic() < timeout_time:
            parent_status_register_data = self.__read(IPC_REGS.MSG_ID_2)

            if (
                done_flag := (parent_status_register_data & IPC_REGS.MSG_ID_2.PARENT_DONE.mask)
                >> IPC_REGS.MSG_ID_2.PARENT_DONE.offset
            ) == 0:
                return
        raise IPCException(
            f"Target did not acknowledge completion of processing the message for {self.polling_timeout} s"
        )

    def __send_random_number(self):
        num = getrandbits(32)
        LOGGER.debug(f"Sent random number ({num})")

        self.write_function(IPC_REGS.PARENT_TO_CPU_0.address, num)
        self.__write(IPC_REGS.MSG_ID_2.PARENT_DONE, 1)
        self.__wait_for_parent_done_unset(time.monotonic() + self.polling_timeout)

    def __bulk_transfer(self, status_register_data: int):
        """Bulk transfer enables to fetch more data in a single transfer.
        First it receives address and size of data in the normal IPC way and
        then performs a single read on the received address and size.
        """
        msg_type = (
            status_register_data & IPC_REGS.MSG_ID_1.MSG_TYPE.mask
        ) >> IPC_REGS.MSG_ID_1.MSG_TYPE.offset

        msg_size = (
            status_register_data & IPC_REGS.MSG_ID_1.MSG_SIZE.mask
        ) >> IPC_REGS.MSG_ID_1.MSG_SIZE.offset

        # clear message size field
        self.__write(IPC_REGS.MSG_ID_1.MSG_SIZE, 0)
        # clear message type field
        self.__write(IPC_REGS.MSG_ID_1.MSG_TYPE, 0)

        # read the bulk data from received address and size with a single transfer
        # temporarily set LogData to false to avoid logging bulk data helper transfer
        self.LogData, log_data_backup = False, self.LogData
        bulk_address, bulk_size_bytes = self.__read_buffer(size=msg_size, type=msg_type)
        self.LogData = log_data_backup

        if bulk_size_bytes > 2*(2**10):
            LOGGER.debug("Bulk transfers bigger than 2KiB might not be received correctly.")

        bulk_data = self.read_function(bulk_address, bulk_size_bytes // 4)

        if self.LogData:
            self.DataLogs.append(bulk_data)
        return bulk_data

    def __poll_status_register(self, method: Literal["session", "wait"]) -> int:
        """Poll MSG_ID_1 status register of IPC and check for activity on CPU_DONE flag.
        Get data from read_buffer function based on message size and type."""

        current_time = time.monotonic()
        timeout_time = current_time + self.polling_timeout

        while current_time < timeout_time:
            status_register_data = self.__read(IPC_REGS.MSG_ID_1)

            _temp = None

            if (
                done_flag := (status_register_data & IPC_REGS.MSG_ID_1.CPU_DONE.mask)
                >> IPC_REGS.MSG_ID_1.CPU_DONE.offset
            ):
                cmd = (
                    status_register_data & IPC_REGS.MSG_ID_1.MSG_CMDS.mask
                ) >> IPC_REGS.MSG_ID_1.MSG_CMDS.offset

                if cmd == IPC_CMDS.PASS:
                    LOGGER.debug(f"Test Passed!")
                    return

                elif cmd == IPC_CMDS.BULK_TRANSFER:
                    _temp = self.__bulk_transfer(status_register_data)

                elif cmd != IPC_CMDS.NONE:
                    # clear command field
                    self.__write(IPC_REGS.MSG_ID_1.MSG_CMDS, 0)
                    self.__execute(cmd)
                    if method == "wait":
                        return

                else:
                    msg_type = (
                        status_register_data & IPC_REGS.MSG_ID_1.MSG_TYPE.mask
                    ) >> IPC_REGS.MSG_ID_1.MSG_TYPE.offset

                    msg_size = (
                        status_register_data & IPC_REGS.MSG_ID_1.MSG_SIZE.mask
                    ) >> IPC_REGS.MSG_ID_1.MSG_SIZE.offset

                    # clear message size field
                    self.__write(IPC_REGS.MSG_ID_1.MSG_SIZE, 0)

                    # clear message type field
                    self.__write(IPC_REGS.MSG_ID_1.MSG_TYPE, 0)

                    _temp = self.__read_buffer(size=msg_size, type=msg_type)

                #! unset done flag after all processing
                self.__write(IPC_REGS.MSG_ID_1.CPU_DONE, 0)

                if _temp:
                    if method == "wait":
                        LOGGER.debug(f"Received: {_temp}")
                        return _temp
                    else:
                        LOGGER.info(f"Received: {_temp}")

            time.sleep(self.polling_period)
            current_time = time.monotonic()

        raise IPCException(f"Did not receive any data after {self.polling_timeout} s")

    def __read(self, register: RegisterProtocol) -> List[int]:
        """Read data

        Args:
            register (int): address data is fetched from.
        Returns:
            data read from the location address.
        """
        return self.read_function(register.address)[0]

    def __write(self, field: FieldProtocol, value: int) -> None:
        """Write data to field.

        Args:
            field (int): address of field data is written to.
            value (int): data to be written.
        """
        register_value = self.__read(field)
        new_register_value = (register_value & ~field.mask) | (value << field.offset)
        self.write_function(field.address, new_register_value)