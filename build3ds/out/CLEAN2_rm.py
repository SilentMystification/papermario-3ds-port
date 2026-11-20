import re,sys
CR = chr(13)
def rm(path, names):
    lines = open(path, encoding='utf-8', newline='').read().split('\n')
    drop = set(); done = []
    for i, l in enumerate(lines):
        if l.rstrip(CR) == '    {' and i + 1 < len(lines):
            m = re.match(r'      "name": "([^"]+)",' + CR + '?$', lines[i + 1])
            if m and m.group(1) in names:
                j = i + 1
                while not re.match(r'    \}(,)?' + CR + '?$', lines[j]):
                    j += 1
                for k in range(i, j + 1):
                    drop.add(k)
                done.append((m.group(1), lines[j].rstrip(CR)))
                if lines[j].rstrip(CR) == '    }':
                    p = i - 1
                    if lines[p].rstrip(CR) == '    },':
                        lines[p] = lines[p].replace('    },', '    }', 1)
    out = [l for k, l in enumerate(lines) if k not in drop]
    open(path, 'w', encoding='utf-8', newline='').write('\n'.join(out))
    return done
print(rm(sys.argv[1], set(sys.argv[2:])))
