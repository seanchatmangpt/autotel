"""Tests for General Utility Helper."""

from autotel.helpers import helpers

def test_deep_merge_dicts():
    a = {"x": 1, "y": {"z": 2, "w": 3}}
    b = {"y": {"z": 99}, "k": 5}
    merged = helpers.deep_merge_dicts(a, b)
    assert merged["x"] == 1
    assert merged["y"]["z"] == 99
    assert merged["y"]["w"] == 3
    assert merged["k"] == 5

def test_flatten_dict():
    d = {"a": {"b": {"c": 1}}, "x": 2}
    flat = helpers.flatten_dict(d)
    assert flat == {"a.b.c": 1, "x": 2}

def test_unflatten_dict():
    flat = {"a.b.c": 1, "x": 2}
    nested = helpers.unflatten_dict(flat)
    assert nested["a"]["b"]["c"] == 1
    assert nested["x"] == 2

def test_safe_get():
    d = {"a": {"b": {"c": 1}}, "x": 2}
    assert helpers.safe_get(d, "a.b.c") == 1
    assert helpers.safe_get(d, "x") == 2
    assert helpers.safe_get(d, "not.found") is None

def test_safe_set():
    d = {"a": {"b": {}}}
    helpers.safe_set(d, "a.b.c", 42)
    assert d["a"]["b"]["c"] == 42
    helpers.safe_set(d, "x.y", 99)
    assert d["x"]["y"] == 99

def test_to_snake_case():
    assert helpers.to_snake_case("CamelCase") == "camel_case"
    assert helpers.to_snake_case("already_snake_case") == "already_snake_case"
    assert helpers.to_snake_case("kebab-case") == "kebab_case"
    assert helpers.to_snake_case("HTTPResponseCode") == "http_response_code"

def test_to_camel_case():
    assert helpers.to_camel_case("snake_case") == "snakeCase"
    assert helpers.to_camel_case("alreadyCamel") == "alreadyCamel"
    assert helpers.to_camel_case("http_response_code") == "httpResponseCode" 