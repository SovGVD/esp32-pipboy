"""Small solid-modelling kit for the inventory's rotating wireframe models.

Models are unions of closed parts (boxes, extrusions, lathed profiles...).
Each part keeps its own vertices; part orientation is fixed automatically so
every face winds counter-clockwise seen from outside. `compile_mesh` turns a
model into the arrays the firmware draws: vertices, face normals, triangles
and edges with their two faces, so the renderer can cull back faces, hide
occluded lines with a depth buffer and draw only silhouettes and sharp edges.
"""
import math

HARD_EDGE_DEGREES = 40
VIEW_PITCH = 0.35  # radians; must match inventory::viewPitch
COORDINATE_LIMIT = 1000
MAX_VERTICES = 1024
MAX_FACES = 1024


class Part:
    def __init__(self, vertices, faces):
        self.vertices = [tuple(float(c) for c in v) for v in vertices]
        self.faces = [list(f) for f in faces]

    def mapped(self, function):
        return Part([function(v) for v in self.vertices], self.faces)


# Transforms ---------------------------------------------------------------

def move(part, x=0, y=0, z=0):
    return part.mapped(lambda v: (v[0]+x, v[1]+y, v[2]+z))


def scale(part, x=1, y=1, z=1):
    return part.mapped(lambda v: (v[0]*x, v[1]*y, v[2]*z))


def rotate(part, axis, degrees):
    c, s = math.cos(math.radians(degrees)), math.sin(math.radians(degrees))
    def turn(v):
        x, y, z = v
        if axis == 'x':
            return (x, y*c - z*s, y*s + z*c)
        if axis == 'y':
            return (x*c + z*s, y, -x*s + z*c)
        return (x*c - y*s, x*s + y*c, z)
    return part.mapped(turn)


def along_x(part):
    """Lay a part built along +Y down along +X."""
    return rotate(part, 'z', -90)


def along_z(part):
    """Point a part built along +Y towards the viewer (+Z)."""
    return rotate(part, 'x', 90)


# Primitives ---------------------------------------------------------------

def box(x0, y0, z0, x1, y1, z1):
    x0, x1 = sorted((x0, x1)); y0, y1 = sorted((y0, y1)); z0, z1 = sorted((z0, z1))
    v = [(x, y, z) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]
    faces = [[0, 1, 3, 2], [4, 6, 7, 5], [0, 4, 5, 1], [2, 3, 7, 6], [0, 2, 6, 4], [1, 5, 7, 3]]
    return Part(v, faces)


def extrude(points, z0, z1):
    """Prism from a simple XY polygon (convex or concave) between z0 and z1."""
    n = len(points)
    vertices = [(x, y, z0) for x, y in points] + [(x, y, z1) for x, y in points]
    faces = [list(range(n)), list(range(n, 2*n))]
    faces += [[i, (i+1) % n, n + (i+1) % n, n + i] for i in range(n)]
    return Part(vertices, faces)


def lathe(profile, segments=12, closed=False):
    """Revolve (radius, y) points around the Y axis.

    Open profiles get flat caps where an end radius is above zero; a zero
    radius closes the end to a point. Closed profiles make rings (holes).
    """
    vertices, rings = [], []
    for r, y in profile:
        if r == 0:
            rings.append([len(vertices)] * segments)
            vertices.append((0, y, 0))
        else:
            rings.append(list(range(len(vertices), len(vertices) + segments)))
            vertices += [(r*math.cos(i*math.tau/segments), y, r*math.sin(i*math.tau/segments)) for i in range(segments)]
    faces = []
    pairs = list(zip(rings, rings[1:])) + ([(rings[-1], rings[0])] if closed else [])
    for a, b in pairs:
        for i in range(segments):
            j = (i+1) % segments
            face = [a[i], a[j], b[j], b[i]]
            unique = []
            for index in face:
                if index not in unique:
                    unique.append(index)
            if len(unique) >= 3:
                faces.append(unique)
    if not closed:
        if profile[0][0] > 0:
            faces.append(list(rings[0]))
        if profile[-1][0] > 0:
            faces.append(list(rings[-1]))
    return Part(vertices, faces)


def cylinder(radius, y0, y1, segments=12):
    return lathe([(radius, y0), (radius, y1)], segments)


def frustum(r0, r1, y0, y1, segments=12):
    return lathe([(r0, y0), (r1, y1)], segments)


def sphere(radius, rings=6, segments=12):
    profile = [(radius*math.sin(math.pi*i/rings), -radius*math.cos(math.pi*i/rings)) for i in range(rings+1)]
    profile[0] = (0, -radius); profile[-1] = (0, radius)
    return lathe(profile, segments)


def ring(inner, outer, y0, y1, segments=16):
    """Washer/tube: a closed lathe with a hole along the Y axis."""
    return lathe([(inner, y0), (outer, y0), (outer, y1), (inner, y1)], segments, closed=True)


def rounded_rectangle(width, height, radius, steps=3):
    points = []
    corners = [(width/2-radius, height/2-radius, 0), (-width/2+radius, height/2-radius, 90),
               (-width/2+radius, -height/2+radius, 180), (width/2-radius, -height/2+radius, 270)]
    for cx, cy, start in corners:
        for i in range(steps+1):
            angle = math.radians(start + 90*i/steps)
            points.append((cx + radius*math.cos(angle), cy + radius*math.sin(angle)))
    return points


# Compilation --------------------------------------------------------------

def _sub(a, b): return (a[0]-b[0], a[1]-b[1], a[2]-b[2])
def _cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def _dot(a, b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]


def _newell(vertices, face):
    n = [0.0, 0.0, 0.0]
    for i, a in enumerate(face):
        p, q = vertices[a], vertices[face[(i+1) % len(face)]]
        n[0] += (p[1]-q[1])*(p[2]+q[2]); n[1] += (p[2]-q[2])*(p[0]+q[0]); n[2] += (p[0]-q[0])*(p[1]+q[1])
    return tuple(n)


def _orient(part):
    """Make neighbouring faces agree, then point every face outwards."""
    faces = [list(f) for f in part.faces]
    owners = {}
    for index, face in enumerate(faces):
        for i, a in enumerate(face):
            b = face[(i+1) % len(face)]
            owners.setdefault(frozenset((a, b)), []).append(index)
    for edge, users in owners.items():
        assert len(users) == 2, f'Part is not a closed surface at edge {sorted(edge)}'
    done, flipped = set(), set()
    for seed in range(len(faces)):
        if seed in done:
            continue
        done.add(seed); stack = [seed]
        while stack:
            index = stack.pop()
            face = faces[index]
            for i, a in enumerate(face):
                b = face[(i+1) % len(face)]
                other = next(u for u in owners[frozenset((a, b))] if u != index)
                if other in done:
                    continue
                o = faces[other]
                same = any(o[k] == a and o[(k+1) % len(o)] == b for k in range(len(o)))
                if same:
                    o.reverse()
                done.add(other); stack.append(other)
    centre = tuple(sum(v[i] for v in part.vertices)/len(part.vertices) for i in range(3))
    volume = 0.0
    for face in faces:
        for i in range(1, len(face)-1):
            a, b, c = (_sub(part.vertices[k], centre) for k in (face[0], face[i], face[i+1]))
            volume += _dot(a, _cross(b, c))
    assert abs(volume) > 1e-9, 'Flat part'
    if volume < 0:
        for face in faces:
            face.reverse()
    return Part(part.vertices, faces)


def _triangulate(vertices, face, normal):
    """Ear clipping in the face plane (caps of extrusions may be concave)."""
    axis = max(range(3), key=lambda i: abs(normal[i]))
    u, v = [(1, 2), (2, 0), (0, 1)][axis]
    sign = 1 if normal[axis] > 0 else -1
    pts = {k: (vertices[k][u], vertices[k][v]) for k in face}
    def area2(a, b, c):
        return sign*((pts[b][0]-pts[a][0])*(pts[c][1]-pts[a][1]) - (pts[b][1]-pts[a][1])*(pts[c][0]-pts[a][0]))
    remaining, result = list(face), []
    while len(remaining) > 3:
        for i in range(len(remaining)):
            a, b, c = remaining[i-1], remaining[i], remaining[(i+1) % len(remaining)]
            if area2(a, b, c) <= 1e-9:
                continue
            others = [k for k in remaining if k not in (a, b, c)]
            if any(area2(a, b, k) >= 0 and area2(b, c, k) >= 0 and area2(c, a, k) >= 0 for k in others):
                continue
            result.append((a, b, c)); remaining.pop(i)
            break
        else:
            raise ValueError('Cannot triangulate face (self-intersecting?)')
    result.append(tuple(remaining))
    return result


def compile_mesh(parts):
    parts = [_orient(p) for p in parts]
    raw = [v for p in parts for v in p.vertices]
    low = [min(v[i] for v in raw) for i in range(3)]
    high = [max(v[i] for v in raw) for i in range(3)]
    centre = [(a+b)/2 for a, b in zip(low, high)]
    factor = COORDINATE_LIMIT / max(max(abs(v[i]-centre[i]) for v in raw) for i in range(3))
    vertices, faces = [], []
    for part in parts:
        base = len(vertices)
        vertices += [tuple(round((v[i]-centre[i])*factor) for i in range(3)) for v in part.vertices]
        faces += [[base + k for k in f] for f in part.faces]
    normals, triangles = [], []
    for index, face in enumerate(faces):
        n = _newell(vertices, face)
        length = math.sqrt(_dot(n, n))
        assert length > 0, 'Degenerate face'
        normals.append(tuple(n[i]/length for i in range(3)))
        triangles += [(index,) + t for t in _triangulate(vertices, face, n)]
    owners = {}
    for index, face in enumerate(faces):
        for i, a in enumerate(face):
            b = face[(i+1) % len(face)]
            owners.setdefault((min(a, b), max(a, b)), []).append(index)
    edges = []
    threshold = math.cos(math.radians(HARD_EDGE_DEGREES))
    for (a, b), (fa, fb) in sorted(owners.items()):
        hard = _dot(normals[fa], normals[fb]) < threshold
        edges.append((a, b, fa, fb, int(hard)))
    assert len(vertices) <= MAX_VERTICES and len(faces) <= MAX_FACES, 'Model too detailed'
    half_width = half_height = 0.0
    cp, sp = math.cos(VIEW_PITCH), math.sin(VIEW_PITCH)
    for step in range(180):
        yaw = step * math.tau / 180
        cy, sy = math.cos(yaw), math.sin(yaw)
        for x, y, z in vertices:
            x1, z1 = x*cy + z*sy, -x*sy + z*cy
            half_width = max(half_width, abs(x1))
            half_height = max(half_height, abs(y*cp - z1*sp))
    return {
        'vertices': vertices,
        'normals': [tuple(round(c*127) for c in n) for n in normals],
        'triangles': triangles,
        'edges': edges,
        'half_width': math.ceil(half_width),
        'half_height': math.ceil(half_height),
    }
