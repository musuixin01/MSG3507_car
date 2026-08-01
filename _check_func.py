import re

with open(r'D:\E_Lab_Base\02_Projects\Keil\MSP3507\MSG3507_car\App\line_follow.c', 'r', encoding='utf-8') as f:
    content = f.read()

# Show the LineFollow_Read function
idx = content.find('LineObservation LineFollow_Read')
start_brace = content.index('{', idx)
brace_count = 0
i = start_brace
while i < len(content):
    if content[i] == '{': brace_count += 1
    elif content[i] == '}':
        brace_count -= 1
        if brace_count == 0:
            break
    i += 1

line_start = content[:idx].count('\n') + 1
line_end = content[:i+1].count('\n') + 1
print(f'LineFollow_Read: lines {line_start} to {line_end} ({line_end-line_start+1} lines)')

# Show first 50 lines and last 30 lines
func_text = content[idx:i+1]
lines = func_text.split('\n')
for l in lines[:50]:
    print(l)
print('...')
for l in lines[-30:]:
    print(l)
