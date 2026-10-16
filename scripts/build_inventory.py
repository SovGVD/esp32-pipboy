#!/usr/bin/env python3
"""Compile editable inventory SVGs/catalog into allocation-free C++ commands.

Supported SVG subset: line, polyline, polygon, rect, circle and ellipse; solid
black/green fills and 2px green strokes. Paths, transforms, external references
and unsupported attributes are rejected instead of silently disappearing.
"""
import argparse
import json
import math
from pathlib import Path
import re
import textwrap
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets/inventory'
DESTINATION = ROOT / 'lib/Inventory/src/InventoryData.h'
PALETTE = {'#000': 0, '#42b610': 1}


def cross(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1]) - (b[1]-a[1])*(c[0]-a[0])


def triangulate(points):
    # Ear clipping supports the concave outlines used by guns and clothing.
    points = list(points)
    area = sum(a[0]*b[1]-b[0]*a[1] for a, b in zip(points, points[1:]+points[:1]))
    assert area != 0, 'Zero-area polygon'
    for i, a in enumerate(points):
        b = points[(i+1) % len(points)]
        for j in range(i+1, len(points)):
            c, d = points[j], points[(j+1) % len(points)]
            assert not (cross(a, b, c)*cross(a, b, d) < 0 and
                        cross(c, d, a)*cross(c, d, b) < 0), 'Self-intersecting polygon'
    if area < 0:
        points.reverse()
    result = []
    while len(points) > 3:
        for i, b in enumerate(points):
            a, c = points[i-1], points[(i+1) % len(points)]
            if cross(a, b, c) <= 0:
                continue
            adjacent = {(i-1) % len(points), i, (i+1) % len(points)}
            others = [p for j, p in enumerate(points) if j not in adjacent]
            if any(cross(a, b, p) >= 0 and cross(b, c, p) >= 0 and cross(c, a, p) >= 0 for p in others):
                continue
            result.append((a, b, c))
            del points[i]
            break
        else:
            raise ValueError('Non-simple or degenerate polygon')
    if len(points) == 3 and cross(*points):
        result.append(tuple(points))
    return result


def compile_svg(path):
    svg = ET.parse(path).getroot()
    assert svg.attrib['viewBox'] == '0 0 200 120', path
    assert set(svg.attrib) <= {'viewBox', 'width', 'height', 'role', 'style'}, path
    assert svg.attrib.get('style', '') in ('', 'background:#000'), path
    commands = []
    for element in svg:
        tag = element.tag.split('}')[-1]
        if tag == 'title':
            continue
        attributes = dict(element.attrib)
        fill = attributes.pop('fill', 'none')
        stroke = attributes.pop('stroke', 'none')
        width = float(attributes.pop('stroke-width', '2'))
        assert width == 2, path
        assert fill == 'none' or fill in PALETTE, path
        assert stroke in ('none', '#42b610'), path
        if tag in ('polygon', 'polyline'):
            values = [float(n) for n in re.split('[ ,]+', attributes.pop('points').strip())]
            assert len(values) % 2 == 0 and len(values) >= 4, path
            points = list(zip(values[::2], values[1::2]))
        elif tag == 'line':
            points = [(float(attributes.pop('x1')), float(attributes.pop('y1'))),
                      (float(attributes.pop('x2')), float(attributes.pop('y2')))]
        elif tag == 'rect':
            x, y, w, h = [float(attributes.pop(key)) for key in ('x', 'y', 'width', 'height')]
            assert w > 0 and h > 0, path
            points = [(x, y), (x+w, y), (x+w, y+h), (x, y+h)]
        elif tag in ('circle', 'ellipse'):
            x, y = float(attributes.pop('cx')), float(attributes.pop('cy'))
            if tag == 'circle':
                rx = ry = float(attributes.pop('r'))
            else:
                rx, ry = float(attributes.pop('rx')), float(attributes.pop('ry'))
            assert rx > 0 and ry > 0, path
            points = [(x+rx*math.cos(i*math.tau/32), y+ry*math.sin(i*math.tau/32)) for i in range(32)]
        else:
            raise ValueError(f'{path}: unsupported SVG element {tag}')
        assert not attributes, (path, attributes)
        assert all(math.isfinite(x) and math.isfinite(y) and 0 <= x <= 200 and 0 <= y <= 120 for x, y in points), path
        if fill != 'none':
            assert tag not in ('line', 'polyline'), path
            for triangle in triangulate(points):
                commands.append((True, PALETTE[fill], [round(v) for point in triangle for v in point]))
        if stroke != 'none':
            edges = zip(points, points[1:]+(points[:1] if tag not in ('line', 'polyline') else []))
            for a, b in edges:
                coordinates = [round(v) for point in (a, b) for v in point] + [0, 0]
                commands.append((False, 1, coordinates))
    assert commands and any(not triangle and paint for triangle, paint, _ in commands), path
    return commands


def generate():
    catalog = json.loads((SOURCE/'catalog.json').read_text())
    assert catalog['setting'] == 'Fallout 4'
    assert [c['label'] for c in catalog['categories']] == ['WEAPONS', 'APPAREL', 'AID', 'MISC', 'JUNK']
    output = ['// Generated by scripts/build_inventory.py. Edit assets/inventory instead.',
              '#pragma once', '', 'namespace inventory {', 'namespace {']
    ids, files = set(), set()
    total = 0
    for category in catalog['categories']:
        assert re.fullmatch('[a-z]+', category['id'])
        assert category['items'] and len(category['items']) <= 65535
        for item in category['items']:
            id = item['id']
            assert re.fullmatch('[a-z0-9]+(?:-[a-z0-9]+)*', id) and id not in ids, id
            ids.add(id)
            assert len(item['name']) <= 20, item['name']
            assert len(item['kind']) <= 23, item['kind']
            assert len(textwrap.wrap(item['description'], 23, break_long_words=True, break_on_hyphens=False)) <= 3, (id, item['description'])
            assert all(32 <= ord(c) <= 126 for key in ('name', 'kind', 'description') for c in item[key]), id
            assert item['source'].startswith('https://fallout.fandom.com/wiki/'), id
            path = (SOURCE/item['model']).resolve()
            assert path.parent == (SOURCE/'svg').resolve(), path
            files.add(path)
            commands = compile_svg(path)
            total += len(commands)
            symbol = id.replace('-', '_')
            output.append(f'constexpr Command model_{symbol}[] = {{')
            for triangle, paint, coordinates in commands:
                output.append('    {' + str(triangle).lower() + ', ' + str(paint) + ', {' + ', '.join(map(str, coordinates)) + '}},')
            output.append('};')
        output.append('constexpr Item items_' + category['id'] + '[] = {')
        for item in category['items']:
            symbol = 'model_' + item['id'].replace('-', '_')
            output.append('    {' + ', '.join(json.dumps(item[key]) for key in ('id', 'name', 'kind', 'description')) +
                          ', {' + symbol + ', sizeof(' + symbol + ') / sizeof(Command)}},')
        output.append('};')
    assert files == set((SOURCE/'svg').glob('*.svg')), 'Missing or orphan SVG models'
    output.append('constexpr Category categories[] = {')
    for category in catalog['categories']:
        symbol = 'items_' + category['id']
        output.append('    {' + json.dumps(category['label']) + ', ' + symbol + ', sizeof(' + symbol + ') / sizeof(Item)},')
    output.extend(['};', 'static_assert(sizeof(categories) / sizeof(Category) == categoryCount, "Inventory category count mismatch");', '}', '}', ''])
    return '\n'.join(output), len(ids), total


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Fail if generated firmware data is stale')
    args = parser.parse_args()
    data, items, commands = generate()
    if args.check:
        assert DESTINATION.read_text() == data, 'Inventory data is stale; run scripts/build_inventory.py'
    else:
        DESTINATION.write_text(data)
    print(f'Inventory: {items} items, {commands} vector commands; generated data ' + ('verified' if args.check else 'written'))
