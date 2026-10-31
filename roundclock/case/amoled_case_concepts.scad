// Waveshare ESP32-S3-Touch-AMOLED-1.43 clock case: 4 CONCEPTS (A..D)
// Concept-stage model for picking a direction. Not yet the final print file.
//
// Model coords: front view, X = right, Y = up, Z = toward the viewer.
// Front face of the case (and of the glass, flush) is z = 0; body goes to -z.
// Screen "up" = board's USB-C on the LEFT when seen from the front.
//
// Usage: openscad -D 'opt="A"' -D 'show_parts=true' -D 'cut=false' ...

opt        = "C2";    // "A" tail, "B" deep can, "C" lean wedge, "D" plinth, "C2" lean, min chin (chosen)
show_parts = true;    // draw the screen module + USB-A jack
cut        = false;   // cut away the +X half (section view)
display    = true;    // rotate to "standing on table" world orientation
part       = "all";   // export one colour group: case glass frame pcb gold black silver white

$fn = 120;
module col(name, hex) { if (part=="all") color(hex) children(); else if (part==name) children(); }

// ---------------- measured / assumed part dimensions ----------------
glass_d   = 46.0;   // front glass OD (Waveshare drawing)
glass_t   = 1.8;    // 0.70 + 1.10 (drawing)
mod_d     = 44.0;   // PCB/frame OD behind the glass (estimated from photo)
pcb_back  = 6.2;    // front of glass -> back of PCB (drawing)
stack     = 10.2;   // front of glass -> tip of M2 H4 standoffs (6.2 + 4.0)
// standoff centres in FRONT-view coords after the 90 deg rotation (USB-C left)
standoffs = [[11.6, -15.75], [11.6, 15.75], [-11.5, -16.25], [-11.5, 16.25]];

// bare right-angle USB-A receptacle (generic AF 90deg THT) - MEASURE ME
jw = 13.1;  jh = 5.7;  jl = 14.0;   // shell width, height, length
jpin = 2.5;                          // pin zone behind the insulator
jpin_up = 3.5;                       // pins/wires rise this far above the shell

// ---------------- print / fit parameters (PLA, 0.4 nozzle) ----------------
fit      = 0.15;                 // radial clearance glass <-> bore
bore_d   = glass_d + 2*fit;      // 46.30
wall     = 1.25;                 // side wall at the glass (3 lines @ 0.42)
R        = bore_d/2 + wall;      // outer radius 24.40  -> OD 48.8
wire_gap = 3.0;                  // room behind standoff tips for bent wires
back_t   = 1.6;
puck_D   = stack + wire_gap + back_t;   // 14.8
flat     = 0.5;                  // flat ground on the bottom of round bodies
jfit     = 0.15;                 // clearance around the jack shell
lip      = 0.8;                  // rear lip that stops the jack pulling out

y_jack   = -22.4;                // jack shell bottom (A, B)
// ---------------------------------------------------------------------------

module glass_module() {
    col("glass","#0b0b0d") translate([0,0,-glass_t]) cylinder(d=glass_d, h=glass_t);
    col("frame","#1d1f24") translate([0,0,-pcb_back+1.2]) cylinder(d=mod_d, h=pcb_back-glass_t-1.2+0.01);
    col("pcb","#1f4fa0") translate([0,0,-pcb_back]) cylinder(d=mod_d-0.6, h=1.2);
    // standoffs
    col("gold","#c9a640") for (p = standoffs) translate([p[0],p[1],-stack]) cylinder(d=3.6, h=stack-pcb_back);
    // headers (bottom one carries 5V/GND), USB-C on the left, misc parts
    col("black","#202020") for (yy=[-15, 15]) translate([-11.5, yy-2.6, -pcb_back-2.5]) cube([18, 5.2, 2.5]);
    col("gold","#d4af37") for (yy=[-16.27,-13.73], xx=[-10.2:2.54:5.2]) translate([xx, yy, -stack+0.6]) cylinder(d=0.64,h=4,$fn=8);
    col("silver","#b8b8b8") translate([-22, -4.5, -pcb_back-3.2]) cube([7.5, 9, 3.2]);
    col("white","#eeeeee") translate([12, -3, -pcb_back-3]) cube([6, 6, 3]);
    col("black","#111111") translate([-4, -4, -pcb_back-1]) cube([8, 8, 1]);
}

// jack in local coords: mouth at z=0, body toward +z, pins at the +z end pointing +y
module usb_a() {
    col("silver","#c8ccd0") difference() {
        translate([-jw/2, 0, 0]) cube([jw, jh, jl]);
        translate([-6, 0.6, -0.01]) cube([12, jh-1.2, jl-3]);
    }
    col("white","#f2f2f2") translate([-5.5, 2.4, 0.5]) cube([11, 1.6, jl-3]);      // tongue
    col("white","#f2f2f2") translate([-jw/2+0.3, 0.3, jl-3]) cube([jw-0.6, jh-0.6, 3+0.6]); // rear insulator
    col("gold","#d8b040") for (i=[0:3]) translate([-3.75+i*2.5, jh-1, jl+0.6]) {
        cube([0.6, 0.6, jpin-0.6]);
        translate([0,0,jpin-1.2]) cube([0.6, jpin_up+1, 0.6]);
    }
    // shell prongs: assumed bent flat against the shell or clipped
}

// cut for a jack whose mouth is at z = zm (facing -z) and shell bottom at y = yb.
// Pocket opens toward +z (into the case) so the jack slides in from inside.
module jack_cut(yb, zm, open_to=0, wz=8) {
    // shell pocket
    translate([-jw/2-jfit, yb-jfit, zm+lip]) cube([jw+2*jfit, jh+2*jfit, jl+jpin+0.5-lip]);
    // mouth opening through the rear lip: clears a plug, smaller than the shell
    translate([-6.3, yb+0.3, zm-0.1]) cube([12.6, jh-0.6, lip+0.2]);
    // pin / wire zone, rising toward the board and open into the cavity
    translate([-jw/2-jfit, yb, zm+jl-0.2]) cube([jw+2*jfit, jh+jpin_up+wz, jpin+0.7+open_to]);
}

// snap latch drawn as a cantilever tab behind the jack's insulator
module jack_latch(yb, zm) {
    col("case","#e8e2d6") translate([-4, yb-0.2-1.2, zm+jl+0.3]) cube([8, 2.2, 1.2]);
}

module bore_and_pegs(back_z) {
    // back_z = z of inner back face (negative)
    difference() {
        translate([0,0,back_z]) cylinder(d=bore_d, h=-back_z+1);
        // pegs that stop the standoffs at z=-stack
        for (p = standoffs) translate([p[0],p[1],back_z-0.1]) cylinder(d=5.5, h=-back_z-stack+0.1);
    }
}
module pushout_holes() {
    for (p = standoffs) translate([p[0],p[1],-200]) cylinder(d=2.4, h=200-stack+0.01, $fn=24);
}
module front_chamfer() {
    // 0.4 mm outer chamfer on the front rim
    // cone starts 0.1 outside the wall so it crosses it cleanly (no tangent faces)
    translate([0,0,-0.5]) difference() {
        cylinder(r=R+1, h=1.1);
        cylinder(r1=R+0.1, r2=R-0.4, h=0.5+0.001);
    }
}
module floor_cut(yt) { translate([-100, yt-100, -100]) cube([200, 100, 200]); }

// ---------------- concept A: thin puck + hidden rear tail ----------------
zmA = -(stack + 0.3 + jpin + jl);   // jack mouth z
module case_A() {
    tail_end = zmA - 0.0;
    difference() {
        union() {
            translate([0,0,-puck_D]) cylinder(r=R, h=puck_D);
            // tail: the bottom slice of the cylinder carried rearward + a sole
            hull() {
                intersection() {
                    translate([0,0,tail_end]) cylinder(r=R, h=-tail_end-2);
                    translate([-50,-R,tail_end]) cube([100, R-(-y_jack-jh-jfit-1.6)-0.0+0, 100]);
                }
                translate([-11, -R+flat, tail_end+1]) cube([22, 1, -tail_end-12]);
            }
        }
        bore_and_pegs(-(stack+wire_gap));
        jack_cut(y_jack, zmA, wz=1);
        pushout_holes();
        front_chamfer();
        floor_cut(-R+flat);
    }
}

// ---------------- concept B: one deep can ----------------
zmB = zmA;
module case_B() {
    D = -zmB;
    difference() {
        union() {
            difference() {
                translate([0,0,-D]) cylinder(r=R, h=D);
                translate([0,0,-D+back_t]) cylinder(d=bore_d, h=D);
            }
            // long posts as standoff stops
            for (p = standoffs) translate([p[0],p[1],-D+0.5]) cylinder(d=5.5, h=D-stack-0.5);
            // jack boss
            intersection() {
                translate([0,0,-D]) cylinder(r=R-0.1, h=D);
                translate([-jw/2-1.8, -R, -D]) cube([jw+3.6, R+y_jack+jh+1.6, jl+jpin+1.5]);
            }
        }
        jack_cut(y_jack, zmB, open_to=2);
        pushout_holes();
        front_chamfer();
        floor_cut(-R+flat);
    }
}

// ---------------- concept C: leaning wedge ----------------
tilt = 12;
module tilted() { translate([0,-R,0]) rotate([-tilt,0,0]) translate([0,R,0]) children(); }
yC   = -R - puck_D*sin(tilt) - 0.6;     // table plane
ybC  = yC + 1.2;                         // jack shell bottom
zmC  = -(jl + jpin + 11);               // jack mouth z
module case_C() {
    difference() {
        union() {
            tilted() translate([0,0,-puck_D]) cylinder(r=R, h=puck_D);
            hull() {
                intersection() {
                    tilted() translate([0,0,-puck_D]) cylinder(r=R, h=puck_D);
                    translate([-50, yC, -60]) cube([100, 10, 100]);
                }
                for (x=[-10, 10]) {
                    translate([x, yC, -3]) rotate([-90,0,0]) cylinder(r=2, h=1, $fn=32);
                    translate([x, yC, zmC+2]) rotate([-90,0,0]) cylinder(r=2, h=jh+2.6, $fn=32);
                }
            }
        }
        tilted() {
            bore_and_pegs(-(stack+wire_gap));
            pushout_holes();
            front_chamfer();
        }
        jack_cut(ybC, zmC, open_to=0);
        // wire path from the jack pins up into the tilted cavity
        translate([-jw/2, ybC+jh, zmC+jl-0.5]) cube([jw, 12, jpin+2]);
        floor_cut(yC);
    }
}

// ---------------- C2: lean-back, chin minimised (Kyle's pick, rev 2) ----------------
// The table plane is only `chin` below the front-bottom rim, so the tilted puck's
// lower back is sliced flat by the table (that slice IS the foot). The cavity floor
// is clipped at `cav_clip` above the rim pivot. The jack moves behind the puck
// into a deeper tail that ramps up into the puck's back.
chin     = 0.5;                          // front rim height above the table
cav_clip = 0.4;                          // cavity floor, above the rim pivot (-R)
yC2      = -R - chin;                    // table plane
ybC2     = yC2 + 1.2;                    // jack shell bottom
// put the jack's insulator end just behind the tilted back face at shell-top height
hC2      = ybC2 + jh + jfit + R;         // shell top, height above the pivot
YC2      = (hC2 + puck_D*sin(tilt)) / cos(tilt);
zjfC2    = -YC2*sin(tilt) - puck_D*cos(tilt) - 0.3;   // jack insulator end z
zmC2     = zjfC2 - jl - lip;             // jack mouth (case back face) z
tail_h   = (ybC2 - yC2) + jh + 2*jfit + 1.4;
module case_C2() {
    difference() {
        union() {
            tilted() translate([0,0,-puck_D]) cylinder(r=R, h=puck_D);
            intersection() {
                hull() {
                    // tall root blended into the puck back, low end carrying the jack mouth
                    translate([-20, yC2, zjfC2-1]) cube([40, 13.5, 1]);
                    translate([-20, yC2, -6]) cube([40, 13.5, 1]);
                    translate([-20, yC2, zmC2]) cube([40, tail_h, 3]);
                }
                // tail stays inside the screen's own cylinder: face-on it's a pure circle
                tilted() translate([0,0,-200]) cylinder(r=R-0.2, h=200-0.5);
            }
        }
        tilted() {
            intersection() {
                bore_and_pegs(-(stack+wire_gap));
                // world half-space above the cavity floor, expressed in the puck frame
                translate([0,-R,0]) rotate([tilt,0,0]) translate([0,R,0])
                    translate([-60, -R+cav_clip, -80]) cube([120, 120, 160]);
            }
            pushout_holes();
            front_chamfer();
        }
        jack_cut(ybC2, zmC2, open_to=0, wz=1);
        // wire channel from the jack pins forward/up into the cavity
        translate([-jw/2, ybC2+jh-0.5, zjfC2-0.5]) cube([jw, 13, 6]);
        floor_cut(yC2);
    }
}

// ---------------- concept D: puck on a plinth ----------------
hb   = 8.0;                       // plinth height
yD   = -R - (hb - 1.75);          // table plane (puck bottom sinks 1.75 into plinth)
ybD  = yD + 1.2;
zmD  = -(stack + jpin + jl);      // pins sit just behind the standoff tips
module case_D() {
    pd = -zmD + 0.0;              // plinth depth
    difference() {
        union() {
            translate([0,0,-puck_D]) cylinder(r=R, h=puck_D);
            hull() for (x=[-12, 12], z=[-1.5, -pd+2])
                translate([x, yD, z]) rotate([-90,0,0]) cylinder(r=2, h=hb, $fn=40);
        }
        bore_and_pegs(-(stack+wire_gap));
        pushout_holes();
        front_chamfer();
        jack_cut(ybD, zmD, open_to=0);
        translate([-jw/2, ybD+jh, zmD+jl-0.5]) cube([jw, 10, jpin+1.5]);
        floor_cut(yD);
    }
}

module the_case() {
    col("case","#e9e4da")
    if (opt=="A") case_A();
    else if (opt=="B") case_B();
    else if (opt=="C") case_C();
    else if (opt=="C2") case_C2();
    else case_D();
}
module the_parts() {
    if (opt=="C" || opt=="C2") tilted() glass_module(); else glass_module();
    if (opt=="A") translate([0, y_jack, zmA+lip]) usb_a();
    if (opt=="B") translate([0, y_jack, zmB+lip]) usb_a();
    if (opt=="C") translate([0, ybC, zmC+lip]) usb_a();
    if (opt=="D") translate([0, ybD, zmD+lip]) usb_a();
    if (opt=="C2") translate([0, ybC2, zmC2+lip]) usb_a();
}
module scene() {
    the_case();
    if (show_parts) the_parts();
}
module maybe_cut() {
    if (cut) difference() { children(); translate([0,-100,-100]) cube([200,200,200]); }
    else children();
}
if (display) rotate([90,0,0]) maybe_cut() scene();
else maybe_cut() scene();
