"""Tests for XML helper utilities."""

import tempfile
import os
from autotel.helpers import xml

SIMPLE_XML = """
<root>
    <item id="1">First</item>
    <item id="2">Second</item>
    <group>
        <item id="3">Third</item>
    </group>
</root>
"""


def test_parse_xml_string():
    root = xml.parse_xml_string(SIMPLE_XML)
    assert root.tag == "root"
    assert len(root.findall("item")) == 2
    assert root.find("group") is not None


def test_parse_xml_file():
    with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
        f.write(SIMPLE_XML)
        temp_file = f.name
    try:
        root = xml.parse_xml_file(temp_file)
        assert root.tag == "root"
        assert len(root.findall("item")) == 2
        assert root.find("group") is not None
    finally:
        os.unlink(temp_file)


def test_get_elements_by_tag():
    root = xml.parse_xml_string(SIMPLE_XML)
    items = xml.get_elements_by_tag(root, "item")
    assert len(items) == 3
    ids = [xml.get_attribute(item, "id") for item in items]
    assert set(ids) == {"1", "2", "3"}


def test_get_element_text():
    root = xml.parse_xml_string(SIMPLE_XML)
    items = xml.get_elements_by_tag(root, "item")
    texts = [xml.get_element_text(item) for item in items]
    assert texts == ["First", "Second", "Third"]


def test_get_attribute():
    root = xml.parse_xml_string(SIMPLE_XML)
    items = xml.get_elements_by_tag(root, "item")
    assert xml.get_attribute(items[0], "id") == "1"
    assert xml.get_attribute(items[1], "id") == "2"
    assert xml.get_attribute(items[2], "id") == "3"
    assert xml.get_attribute(items[0], "missing") is None


def test_pretty_print_xml():
    root = xml.parse_xml_string(SIMPLE_XML)
    pretty = xml.pretty_print_xml(root)
    assert pretty.startswith("<root>")
    assert "<item id=\"1\">First</item>" in pretty
    assert "<group>" in pretty 