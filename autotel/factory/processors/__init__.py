# Compatibility layer: expose processors under autotel.factory.processors
from autotel.processors import (
    BPMNProcessor,
    DMNProcessor,
    DSPyProcessor,
    SHACLProcessor,
    OWLProcessor,
    OTELProcessor,
    JinjaProcessor,
    SPARQLProcessor,
)

__all__ = [
    'BPMNProcessor','DMNProcessor','DSPyProcessor','SHACLProcessor',
    'OWLProcessor','OTELProcessor','JinjaProcessor','SPARQLProcessor'
]

