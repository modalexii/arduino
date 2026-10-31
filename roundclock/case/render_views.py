"""Render the 4 case concepts from STLs exported by export.sh into one comparison sheet."""
import os, sys
os.environ["PYOPENGL_PLATFORM"] = "egl"
import numpy as np, trimesh, pyrender
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
COL = {"case": "#d8d0c0", "glass": "#0c0c0f", "frame": "#26282e", "pcb": "#2a5db0",
       "gold": "#d1ad45", "black": "#1a1a1a", "silver": "#b9bec4", "white": "#f4f4f4", "red": "#c8252c"}
TILE = 620

def hex2rgba(h, a=1.0):
    h = h.lstrip("#"); return [int(h[i:i+2], 16)/255 for i in (0, 2, 4)] + [a]

def look_at(eye, target, up=(0, 0, 1)):
    eye, target, up = map(np.array, (eye, target, up))
    f = target - eye; f = f/np.linalg.norm(f)
    s = np.cross(f, up); s = s/np.linalg.norm(s)
    u = np.cross(s, f)
    m = np.eye(4); m[:3, 0] = s; m[:3, 1] = u; m[:3, 2] = -f; m[:3, 3] = eye
    return m

def load(opt, cut, ground_z):
    meshes = []
    for p, c in COL.items():
        f = f"{HERE}/stl/{opt}_{p}_{'true' if cut else 'false'}.stl"
        if not os.path.exists(f) or os.path.getsize(f) < 200: continue
        try: m = trimesh.load(f)
        except Exception: continue
        if m.is_empty: continue
        rough = 0.35 if p == "glass" else 0.7
        metal = 0.6 if p in ("silver", "gold") else 0.0
        mat = pyrender.MetallicRoughnessMaterial(baseColorFactor=hex2rgba(c), roughnessFactor=rough, metallicFactor=metal)
        meshes.append(pyrender.Mesh.from_trimesh(m, material=mat, smooth=False))
        if cut and p == "case":   # darker cap on the section plane so voids read clearly
            cap = section_cap(f"{HERE}/stl/{opt}_case_false.stl")
            if cap is not None:
                cm = pyrender.MetallicRoughnessMaterial(baseColorFactor=hex2rgba("#a99c84"), roughnessFactor=0.9, metallicFactor=0.0)
                meshes.append(pyrender.Mesh.from_trimesh(cap, material=cm, smooth=False))
    return meshes

CUT_X = float(os.environ.get("CUT_X", "0"))

def section_cap(full_stl):
    """Thin slab of the case's x=0 cross-section, sitting just on the visible side."""
    m = trimesh.load(full_stl)
    sec = m.section(plane_origin=[CUT_X + 0.02, 0, 0], plane_normal=[1, 0, 0])
    if sec is None: return None
    planar, to3d = sec.to_2D()
    parts = [trimesh.creation.extrude_polygon(pg, 0.05) for pg in planar.polygons_full]
    if not parts: return None
    cap = trimesh.util.concatenate(parts); cap.apply_transform(to3d)
    if np.mean(cap.vertices[:, 0]) < CUT_X: cap.apply_translation([0.06, 0, 0])
    return cap

def render(opt, view, cut=False):
    scene = pyrender.Scene(bg_color=[0.965, 0.962, 0.955, 1.0], ambient_light=[0.22, 0.22, 0.22])
    for m in load(opt, cut, 0): scene.add(m)
    # bounds (all options sit with the table plane at their own min z)
    tgt = np.array([0, 13, -2.0])
    if view == "front":
        cam = pyrender.OrthographicCamera(xmag=33, ymag=33, znear=1, zfar=1000); pose = look_at([0, -300, -4], [0, 0, -4])
    elif view == "side":
        cam = pyrender.OrthographicCamera(xmag=33, ymag=33, znear=1, zfar=1000); pose = look_at([300, 13, -4], [0, 13, -4])
    elif view == "faceon":   # perpendicular to a 12-degree lean-back screen
        t = np.radians(12); c = np.array([0, 24.4*np.sin(t), -24.4 + 24.4*np.cos(t)]) + np.array([0, 0, -3])
        cam = pyrender.OrthographicCamera(xmag=33, ymag=33, znear=1, zfar=1000)
        pose = look_at(c + 300*np.array([0, -np.cos(t), np.sin(t)]), c)
    elif view == "zoom":     # close section on the USB pocket
        cam = pyrender.OrthographicCamera(xmag=14, ymag=14, znear=1, zfar=1000); pose = look_at([300, 19, -15], [0, 19, -15])
    elif view == "rearzoom":
        cam = pyrender.PerspectiveCamera(yfov=np.radians(30), znear=1, zfar=1000); pose = look_at([80, 120, 10], [0, 28, -19])
    elif view == "intozoom":  # looking into the open front at the bottom-rear of the cavity
        cam = pyrender.PerspectiveCamera(yfov=np.radians(28), znear=1, zfar=1000); pose = look_at([-30, -95, 55], [0, 14, -17])
    elif view == "print":    # print orientation, front face on the bed
        cam = pyrender.PerspectiveCamera(yfov=np.radians(30), znear=1, zfar=1000); pose = look_at([95, -110, 85], [0, 0, 12])
    elif view == "bottom":   # from underneath, to show the release holes
        cam = pyrender.PerspectiveCamera(yfov=np.radians(30), znear=1, zfar=1000); pose = look_at([0, 16, -200], [0, 16, -24], up=(0, -1, 0)); cam = pyrender.OrthographicCamera(xmag=30, ymag=30, znear=1, zfar=1000)
    elif view == "mouth":    # straight at the USB-C opening, along the screen axis
        t = np.radians(12); c = np.array([0, 28.76, -19.9])
        cam = pyrender.OrthographicCamera(xmag=9, ymag=9, znear=1, zfar=1000)
        pose = look_at(c + 80*np.array([0, np.cos(t), -np.sin(t)]), c)
    elif view == "rear":
        cam = pyrender.PerspectiveCamera(yfov=np.radians(30), znear=1, zfar=1000); pose = look_at([105, 125, 35], [0, 13, -6])
    elif view == "front34":
        cam = pyrender.PerspectiveCamera(yfov=np.radians(30), znear=1, zfar=1000); pose = look_at([-95, -135, 30], [0, 10, -6])
    scene.add(cam, pose=pose)
    # key + fill + rim lights relative to camera
    for d, i in (((-0.5, 0.6, 1), 2.2), ((0.9, -0.2, 0.5), 0.9), ((0.2, -1, 0.2), 0.5)):
        L = pyrender.DirectionalLight(color=np.ones(3), intensity=i)
        lp = pose.copy(); dd = np.array(d, float); dd /= np.linalg.norm(dd)
        z = pose[:3, :3] @ dd
        lp = look_at(z*100 + pose[:3, 3]*0, [0, 0, 0]); scene.add(L, pose=lp)
    r = pyrender.OffscreenRenderer(TILE, TILE)
    img, _ = r.render(scene, flags=0)
    r.delete()
    return Image.fromarray(img)

if __name__ == "__main__":
    opt, view = sys.argv[1], sys.argv[2]
    cut = len(sys.argv) > 3 and sys.argv[3] == "cut"
    render(opt, view, cut).save(f"{HERE}/r/{opt}_{view}{'_cut' if cut else ''}.png")
