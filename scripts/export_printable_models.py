from dataclasses import dataclass
from typing import Any, Callable, Final, Optional, Self
from pathlib import Path

import freecad

@dataclass(kw_only=True)
class ModelRecord:
    shape: Any
    label: str
    instances: int

    @classmethod
    def from_freecad_object(cls, o: Any) -> Optional[Self]:        
        def _proxy(b: Any) -> Optional[Any]:
            return b
        
        def _get_body_from_part(p: Any) -> Optional[Any]:
            bodies = tuple(filter(
                (lambda o: o.TypeId.startswith("Part")),
                p.Group,    
            ))

            if len(bodies) == 0:
                print(f"expected at least one Body inside part: {p.Label}")
                return None
            
            return bodies[-1]

        
        body_get_strategies: Final[dict[str, Callable[[Any], Optional[Any]]]] = {
            "App::Part" : _get_body_from_part,
            "PartDesign::Body" : _proxy,
            "Part::Feature" : _proxy,
        }

        strategy = body_get_strategies.get(o.TypeId)
        if strategy is None:
            print(f"unsupported {o.TypeId=}")
            return None
        
        body = strategy(o)
        if body is None:
            print(f"failed to get body from {o.Label}")
            return None

        return cls(
            shape=body.Shape,
            label=o.Label,
            instances=1,
        )


def open_document(document_path: Path) -> Optional[Any]:
    try:
        return freecad.app.openDocument(str(document_path))

    except OSError as e:
        print(f"error: {e}")
        return None


def get_property(o: Any, property_name: str) -> Optional[Any]:
    if property_name in o.PropertiesList:
        return o.getPropertyByName(property_name)
    
    return None


def is_printable(o: Any) -> bool:
    return bool(get_property(o, "Printable")) is True


def _start():
    repo_dir: Final = Path(__file__).resolve().parent.parent
    artifacts_dir = repo_dir / "artifacts"
    print(f"{repo_dir=}")

    botix = open_document(repo_dir / "botix.fcstd")

    # 1. fill printable models registry
    
    printable_objects = filter(
        is_printable,
        botix.Objects,
    )

    successful_records = filter(
        (lambda r: r is not None), 
        map(ModelRecord.from_freecad_object, printable_objects)
    )

    model_record_registry: Final[dict[str, ModelRecord]] = {
        record.label: record 
        for record in successful_records
    }
    
    # 2. Resolve instance count via link counter

    all_links = filter(
        (lambda o: o.TypeId == "App::Link"), 
        botix.Objects,
    )

    for link in all_links:
        linked_label = link.getLinkedObject().Label
        if linked_label in model_record_registry:
            model_record_registry[linked_label].instances += 1

    # 3. export STL models

    print('\n'.join((
        f"{k} : {v}"
        for k, v in model_record_registry.items()
    )))

    for record in model_record_registry.values():
        record.shape.exportStep(str(artifacts_dir / f"{record.label}.stp"))

    return


if __name__ == "__main__":
    _start()