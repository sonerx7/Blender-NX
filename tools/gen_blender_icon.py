import math
from PIL import Image, ImageDraw, ImageFilter

def create_blender_icon(path):
    S = 1024  # 4x supersampling for crisp 256x256 anti-aliasing
    img = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # 1. Dark metallic squircle background with subtle inner rim
    pad = 32
    radius = 192
    draw.rounded_rectangle([pad, pad, S - pad, S - pad], radius=radius,
                           fill=(26, 28, 34, 255), outline=(62, 68, 82, 255), width=10)

    # Subtle ambient orange-blue radial glow behind logo
    glow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    gdraw = ImageDraw.Draw(glow)
    gdraw.ellipse([180, 200, S - 180, S - 180], fill=(234, 118, 0, 55))
    glow = glow.filter(ImageFilter.GaussianBlur(95))
    img = Image.alpha_composite(img, glow)

    # 2. Shadow layer for Blender logo
    shadow = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    sdraw = ImageDraw.Draw(shadow)

    cx, cy = 545, 535  # Slightly right-shifted so left arms balance the composition
    orange = (234, 118, 0, 255)
    orange_hi = (255, 152, 38, 255)
    white = (252, 252, 255, 255)
    blue = (36, 118, 206, 255)
    blue_hi = (72, 158, 242, 255)

    # Authentic Blender 3-arm angles (pointing upper-left, left, lower-left)
    arms = [
        (-138, 310, 76),
        (178,  345, 76),
        (134,  310, 76),
    ]

    def draw_logo_shape(d, ox, oy, col_main):
        for angle_deg, length, width in arms:
            rad = math.radians(angle_deg)
            ex = (cx + ox) + length * math.cos(rad)
            ey = (cy + oy) + length * math.sin(rad)
            d.line([(cx + ox, cy + oy), (ex, ey)], fill=col_main, width=width)
            r_tip = width / 2.0
            d.ellipse([ex - r_tip, ey - r_tip, ex + r_tip, ey + r_tip], fill=col_main)
        R_outer = 215
        d.ellipse([cx + ox - R_outer, cy + oy - R_outer, cx + ox + R_outer, cy + oy + R_outer], fill=col_main)

    draw_logo_shape(sdraw, 0, 22, (0, 0, 0, 140))
    shadow = shadow.filter(ImageFilter.GaussianBlur(28))
    img = Image.alpha_composite(img, shadow)

    # 3. Main Blender Orange Logo
    logo = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    ldraw = ImageDraw.Draw(logo)
    draw_logo_shape(ldraw, 0, 0, orange)

    # Subtle 3D highlight on upper-left of orange ring
    R_hi = 200
    ldraw.arc([cx - R_hi, cy - R_hi, cx + R_hi, cy + R_hi], start=200, end=330, fill=orange_hi, width=16)

    # Inner white circle
    R_white = 136
    ldraw.ellipse([cx - R_white, cy - R_white, cx + R_white, cy + R_white], fill=white)

    # Center blue iris
    R_blue = 86
    ldraw.ellipse([cx - R_blue, cy - R_blue, cx + R_blue, cy + R_blue], fill=blue)

    # Specular highlight on blue iris
    ldraw.ellipse([cx - 50, cy - 52, cx + 6, cy + 4], fill=blue_hi)
    ldraw.ellipse([cx - 32, cy - 38, cx - 10, cy - 16], fill=(255, 255, 255, 190))

    img = Image.alpha_composite(img, logo)

    # 4. Crisp modern "NX" badge at bottom-right
    bdraw = ImageDraw.Draw(img)
    bx1, by1, bx2, by2 = 716, 756, 944, 904
    bdraw.rounded_rectangle([bx1 + 6, by1 + 10, bx2 + 6, by2 + 10], radius=38, fill=(0, 0, 0, 130))
    bdraw.rounded_rectangle([bx1, by1, bx2, by2], radius=38, fill=(224, 16, 32, 255), outline=(255, 120, 120, 180), width=6)

    # Bold "N"
    w_s = 16
    nx1, nx2 = 766, 822
    ny1, ny2 = 792, 868
    bdraw.line([(nx1, ny2), (nx1, ny1)], fill=white, width=w_s)
    bdraw.line([(nx1, ny1), (nx2, ny2)], fill=white, width=w_s)
    bdraw.line([(nx2, ny2), (nx2, ny1)], fill=white, width=w_s)
    # Bold "X"
    xx1, xx2 = 852, 908
    bdraw.line([(xx1, ny1), (xx2, ny2)], fill=white, width=w_s)
    bdraw.line([(xx2, ny1), (xx1, ny2)], fill=white, width=w_s)

    # Downsample 1024 -> 256 with LANCZOS
    final_img = img.resize((256, 256), Image.Resampling.LANCZOS)
    final_img.save(path)
    print(f"Generated high-res icon at {path} ({final_img.size})")

if __name__ == '__main__':
    create_blender_icon('nexvyn-blender/icon.png')
