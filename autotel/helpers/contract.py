"""
AutoTel Contract/ABC Helper Utilities

Provides Abstract Base Classes and contract decorators for processor unification.
Happy path only, with type hints and docstrings.
"""

from abc import ABC, abstractmethod
from typing import Any, Callable, Type
import icontract
import wrapt

class BaseProcessorABC(ABC):
    """
    Abstract base class for all processors.
    Enforces a standard interface.
    """
    @abstractmethod
    def process(self, data: Any) -> Any:
        """Process input data and return result."""
        pass

    @abstractmethod
    def get_name(self) -> str:
        """Return the processor's name."""
        pass


def contract_precondition(condition: Callable[..., bool]) -> Callable:
    """
    Decorator for preconditions using icontract.
    Args:
        condition: A function that returns True if precondition is met
    Returns:
        Decorator
    """
    return icontract.require(condition)


def contract_postcondition(condition: Callable[..., bool]) -> Callable:
    """
    Decorator for postconditions using icontract.
    Args:
        condition: A function that returns True if postcondition is met
    Returns:
        Decorator
    """
    return icontract.ensure(condition)


def apply_contracts_dynamically(func: Callable, pre: Callable = None, post: Callable = None) -> Callable:
    """
    Dynamically apply pre/postcondition contracts to a function using wrapt.
    Args:
        func: The function to wrap
        pre: Precondition callable (returns True if allowed)
        post: Postcondition callable (returns True if allowed)
    Returns:
        Wrapped function
    """
    @wrapt.decorator
    def wrapper(wrapped, instance, args, kwargs):
        if pre:
            assert pre(*args, **kwargs)
        result = wrapped(*args, **kwargs)
        if post:
            assert post(result)
        return result
    return wrapper(func)


def check_contract_compliance(cls: Type) -> bool:
    """
    Check if a class implements all abstract methods of BaseProcessorABC.
    Args:
        cls: The class to check
    Returns:
        True if compliant, False otherwise
    """
    return issubclass(cls, BaseProcessorABC) and not bool(getattr(cls, "__abstractmethods__", None)) 