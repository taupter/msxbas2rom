import json
import sys

THRESHOLD = 80.0

report_path = sys.argv[1] if len(sys.argv) > 1 else 'mutation_report.json'

try:
    with open(report_path, 'r') as f:
        data = json.load(f)
except FileNotFoundError:
    print(f'❌ Mutation report not found: {report_path}. Run mutation-run first.')
    sys.exit(1)
except json.JSONDecodeError:
    print(f'❌ Invalid JSON in {report_path}.')
    sys.exit(1)

total = 0
killed = 0
statuses = {}
for file_result in data.get('files', {}).values():
    for mutant in file_result.get('mutants', []):
        total += 1
        status = mutant.get('status')
        statuses[status] = statuses.get(status, 0) + 1
        if status == 'Killed':
            killed += 1

if total == 0:
    print('⚠️  No mutants found. Check if the mutation binary was built with the mull front-end plugin.')
    sys.exit(1)

score = data.get('mutationScore')
if score is None:
    score = (killed / total) * 100.0

print(f'📊 Total mutants: {total}')
for status, count in sorted(statuses.items()):
    print(f'   - {status}: {count}')
print(f'📈 Mutation Score: {score:.2f}%')

if score >= THRESHOLD:
    print(f'✅ Mutation score is above {THRESHOLD:.0f}% - PASSED!')
    sys.exit(0)
else:
    print(f'❌ Mutation score is {score:.2f}% - BELOW {THRESHOLD:.0f}% threshold!')
    print('💡 Improve your tests to catch more mutants.')
    sys.exit(1)
