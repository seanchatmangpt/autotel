"""Payment settlement evidence: OTel spans -> OCEL 2.0 + reconciliation classification.

Standard library only. Deliberately outside the `autotel` package so it imports
without the heavy runtime dependencies.
"""
from .otel_to_ocel import (  # noqa: F401
    Finding,
    EffectView,
    analyze,
    to_ocel,
)
