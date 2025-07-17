"""Tests for Persistence/State Helper."""

import tempfile
import os
import pickle
from autotel.helpers import persistence


def test_save_load_json():
    data = {"a": 1, "b": [2, 3]}
    with tempfile.NamedTemporaryFile(mode='w+', suffix='.json', delete=False) as f:
        file_path = f.name
    try:
        persistence.save_json(data, file_path)
        loaded = persistence.load_json(file_path)
        assert loaded == data
    finally:
        os.unlink(file_path)


def test_save_load_yaml():
    if persistence.yaml is None:
        return  # Skip if PyYAML not available
    data = {"a": 1, "b": [2, 3]}
    with tempfile.NamedTemporaryFile(mode='w+', suffix='.yaml', delete=False) as f:
        file_path = f.name
    try:
        persistence.save_yaml(data, file_path)
        loaded = persistence.load_yaml(file_path)
        assert loaded == data
    finally:
        os.unlink(file_path)


def test_save_load_pickle():
    data = {"a": 1, "b": [2, 3]}
    with tempfile.NamedTemporaryFile(suffix='.pkl', delete=False) as f:
        file_path = f.name
    try:
        persistence.save_pickle(data, file_path)
        loaded = persistence.load_pickle(file_path)
        assert loaded == data
    finally:
        os.unlink(file_path)


def test_atomic_write():
    content = "hello world"
    with tempfile.NamedTemporaryFile(mode='w+', suffix='.txt', delete=False) as f:
        file_path = f.name
    try:
        persistence.atomic_write(content, file_path)
        with open(file_path, 'r', encoding='utf-8') as f2:
            assert f2.read() == content
    finally:
        os.unlink(file_path) 