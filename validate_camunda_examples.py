from SpiffWorkflow.camunda.parser import CamundaParser
from lxml import etree

# Paths to example files
BPMN_FILE = 'bpmn/camunda_five_pillars_simple.bpmn'
DMN_FILE = 'bpmn/quality_decision.dmn'

parser = CamundaParser()

# Load BPMN file
with open(BPMN_FILE, 'r', encoding='utf-8') as f:
    bpmn_tree = etree.parse(f)
    parser.add_bpmn_xml(bpmn_tree.getroot(), BPMN_FILE)

print('--- Parsed BPMN Processes ---')
for process_id, spec in parser.find_all_specs().items():
    print(f'Process ID: {process_id}, Name: {getattr(spec, "name", "<no name>")})')

# Load DMN file
with open(DMN_FILE, 'r', encoding='utf-8') as f:
    dmn_tree = etree.parse(f)
    parser.add_dmn_xml(dmn_tree.getroot(), DMN_FILE)

print('\n--- Parsed DMN Decisions ---')
for dmn_id, dmn_parser in parser.dmn_parsers.items():
    dmn_parser.parse()
    decision = dmn_parser.decision
    print(f'Decision ID: {decision.id}, Name: {decision.name}') 