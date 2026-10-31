// =====================================================================
//  Round AMOLED clock case  -  final design "C2" (lean-back, minimal chin)
//  Board: Waveshare ESP32-S3-Touch-AMOLED-1.43 (46 mm round glass)
//  Power: bare right-angle THT USB-A female, loaded from the back,
//         locked by bending its own pins up inside the case.
//  One piece, no screws. PLA, 0.4 nozzle. See CASE_NOTES.md.
//
//  mode = "print"    -> the case only, in print orientation (front face on
//                       the bed). Export this to STL.
//  mode = "assembly" -> standing on the table, with the board + jack, for
//                       renders (part / cut select colour groups / section).
// =====================================================================

mode = "print";
part = "all";        // assembly colour group: case glass frame pcb gold black silver white
cut  = false;        // assembly: remove the +X half (section view)
show_module = true;  // assembly: draw the screen module
jack_dz     = 0;     // assembly: pull the jack out the back by this much
pins_bent   = true;  // assembly: jack pins bent up (installed) or straight

$fn = 160;

// ---------------- part dimensions (CHECK WITH CALIPERS BEFORE SLICING) -------
glass_d   = 46.0;   // front glass OD                      (Waveshare drawing)
glass_t   = 1.8;    // glass + panel thickness            (drawing: 0.70 + 1.10)
mod_d     = 44.0;   // PCB/frame OD behind the glass       ** MEASURE ** (photo est.)
pcb_back  = 6.2;    // glass front -> PCB back            (drawing)
stack     = 10.2;   // glass front -> standoff tips (6.2 + H4 standoffs)
// standoff centres, front view, board turned so USB-C is on the LEFT  ** MEASURE **
standoffs = [[11.6, -15.75], [11.6, 15.75], [-11.5, -16.25], [-11.5, 16.25]];

jw = 13.1;          // USB-A shell width                   ** MEASURE **
jh = 5.7;           // USB-A shell height                  ** MEASURE **
jl = 14.0;          // USB-A shell length, mouth -> back   ** MEASURE **
jins = 0.7;         // insulator standing proud of the shell's back edge
jpin = 2.5;  jpin_up = 3.5;   // (preview only) pin run + rise

// ---------------- fit / print parameters ----------------
fit      = 0.15;               // radial clearance, glass in bore
bore_d   = glass_d + 2*fit;    // 46.30
bore_r   = bore_d/2;
wall     = 1.25;               // side wall at the glass
R        = bore_r + wall;      // 24.40 -> 48.8 OD
wire_gap = 3.0;                // behind the standoff tips
back_t   = 1.6;
puck_D   = stack + wire_gap + back_t;   // 14.8
ceil_z   = -(stack + wire_gap);         // inner back face (puck frame)
jfit     = 0.15;               // USB-A pocket clearance (crush ribs add 0.15 interference)
stop_t   = 1.0;                // USB-A stop wall
pad_d    = 5.0;                // standoff stop pad
pushout_d= 2.4;                // push-out holes behind the stop pads
chamf_o  = 0.3;                // outer front-rim chamfer
chamf_i  = 0.2;                // inner front-rim chamfer (lead-in + elephant-foot relief)

// ---------------- stance ----------------
tilt     = 12;                 // screen leans back
chin     = 0.5;                // front rim height above the table
cav_clip = 0.4;                // cavity floor height above the rim pivot

// ---------------- derived placement ----------------
y_tab  = -R - chin;                               // table plane (world)
yb     = y_tab + 1.2;                             // jack shell bottom
h_top  = yb + jh + jfit + R;                      // shell top above the pivot
Yt     = (h_top + puck_D*sin(tilt)) / cos(tilt);
z_jf   = -Yt*sin(tilt) - puck_D*cos(tilt) - 0.3;  // jack back end (insulator side)
z_m    = z_jf - jl;                               // jack mouth = case back face
tail_h = (yb - y_tab) + jh + 2*jfit + 2.0;           // 2 mm over the jack

// world <- puck frame (rim pivot at y = -R, z = 0)
module tilted()   { translate([0,-R,0]) rotate([-tilt,0,0]) translate([0,R,0]) children(); }
module untilted() { translate([0,-R,0]) rotate([ tilt,0,0]) translate([0,R,0]) children(); }
module col(name, hex) { if (part=="all") color(hex) children(); else if (part==name) children(); }

// ===================== case (world coords) =====================
module body() {
    tilted() translate([0,0,-puck_D]) cylinder(r=R, h=puck_D);
    intersection() {
        hull() {   // tail: tall root blended into the puck back, low end at the jack mouth
            translate([-20, y_tab, -6])       cube([40, 13.5, 1]);
            translate([-20, y_tab, z_jf-1])   cube([40, 13.5, 1]);
            translate([-20, y_tab, z_m])      cube([40, tail_h, 3]);
        }
        // stays inside the screen's own cylinder -> face-on it's a pure circle
        tilted() translate([0,0,-200]) cylinder(r=R-0.2, h=200-0.5);
    }
}

// standoff stop: a flat pad at the standoff tip (z = -stack), joined to the
// side wall so it isn't a floating island when printed face-down. The contact
// face must stay flat at -stack, so the joint to the wall is flat too.
module stop_corbel(p) {
    rp = norm(p); u = p / rp;
    hull() {
        translate([p[0], p[1], ceil_z-0.1]) cylinder(d=pad_d, h=(-stack)-(ceil_z-0.1), $fn=40);
        translate([u[0]*(bore_r+0.6), u[1]*(bore_r+0.6), ceil_z-0.1])
            cylinder(d=pad_d, h=(-stack)-(ceil_z-0.1), $fn=40);
    }
}

module cavity() {   // puck frame
    intersection() {
        difference() {
            translate([0,0,ceil_z]) cylinder(r=bore_r, h=-ceil_z+1);
            for (p = standoffs) stop_corbel(p);
        }
        // keep a floor: nothing below cav_clip (world) is cut
        untilted() translate([-60, -R+cav_clip, -100]) cube([120, 120, 200]);
    }
    // inner front chamfer
    translate([0,0,-chamf_i]) cylinder(r1=bore_r, r2=bore_r+chamf_i+0.01, h=chamf_i+0.01);
    translate([0,0,-0.001]) cylinder(r=bore_r+chamf_i+0.01, h=1);
}

module front_chamfer() {   // puck frame; cone starts outside the wall so faces cross cleanly
    translate([0,0,-chamf_o-0.1]) difference() {
        cylinder(r=R+1, h=chamf_o+1.1);
        cylinder(r1=R+0.1, r2=R-chamf_o, h=chamf_o+0.1+0.001);
    }
}

module pushout_holes() {   // puck frame
    for (p = standoffs) translate([p[0],p[1],-100]) cylinder(d=pushout_d, h=100-stack+0.01, $fn=24);
}

// USB-A pocket: loads from the back, pins exit through a slot in the stop wall
module jack_pocket() {
    difference() {
        translate([-jw/2-jfit, yb-jfit, z_m-1]) cube([jw+2*jfit, jh+2*jfit, jl+jins+1]);
        // crush ribs (0.15 interference)
        for (sx=[-1,1], fy=[0.3, 0.75]) translate([sx*(jw/2+jfit), yb+jh*fy, z_m+1.5]) cylinder(r=0.3, h=jl-1.5, $fn=12);
        for (x=[-4, 4]) translate([x, yb-jfit, z_m+1.5]) cylinder(r=0.3, h=jl-1.5, $fn=12);
    }
    hull() {   // lead-in
        translate([-jw/2-jfit-0.5, yb-jfit-0.5, z_m-0.01]) cube([jw+2*jfit+1.0, jh+2*jfit+1.0, 0.01]);
        translate([-jw/2-jfit, yb-jfit, z_m+0.5]) cube([jw+2*jfit, jh+2*jfit, 0.01]);
    }
    translate([-5.25, yb+jh-3.2, z_m+jl+jins-0.1]) cube([10.5, 3.2, stop_t+0.2]);   // pin slot
    translate([-jw/2, yb+jh-3.2, z_m+jl+jins+stop_t]) cube([jw, 16, 5]);           // wire channel
}

module case_world() {
    difference() {
        body();
        tilted() { cavity(); pushout_holes(); front_chamfer(); }
        jack_pocket();
        translate([-100, y_tab-100, -100]) cube([200, 100, 200]);   // table
    }
}

// ===================== preview parts =====================
module glass_module() {   // puck frame
    col("glass","#0b0b0d") translate([0,0,-glass_t]) cylinder(d=glass_d, h=glass_t);
    col("frame","#1d1f24") translate([0,0,-pcb_back+1.2]) cylinder(d=mod_d, h=pcb_back-glass_t-1.2+0.01);
    col("pcb","#1f4fa0") translate([0,0,-pcb_back]) cylinder(d=mod_d-0.6, h=1.2);
    col("gold","#c9a640") for (p = standoffs) translate([p[0],p[1],-stack]) cylinder(d=3.6, h=stack-pcb_back);
    col("black","#202020") for (yy=[-15, 15]) translate([-11.5, yy-2.6, -pcb_back-2.5]) cube([18, 5.2, 2.5]);
    col("gold","#d4af37") for (yy=[-16.27,-13.73], xx=[-10.2:2.54:5.2]) translate([xx, yy, -stack+0.6]) cylinder(d=0.64,h=4,$fn=8);
    col("silver","#b8b8b8") translate([-22, -4.5, -pcb_back-3.2]) cube([7.5, 9, 3.2]);
    col("white","#eeeeee") translate([12, -3, -pcb_back-3]) cube([6, 6, 3]);
    col("black","#111111") translate([-4, -4, -pcb_back-1]) cube([8, 8, 1]);
}
module usb_a(bent=true) {  // mouth at z=0, body toward +z, pins toward +y
    col("silver","#c8ccd0") difference() {
        translate([-jw/2, 0, 0]) cube([jw, jh, jl]);
        translate([-6, 0.6, -0.01]) cube([12, jh-1.2, jl-3]);
    }
    col("white","#f2f2f2") translate([-5.5, 2.4, 0.5]) cube([11, 1.6, jl-3]);
    col("white","#f2f2f2") translate([-jw/2+0.3, 0.3, jl-3]) cube([jw-0.6, jh-0.6, 3+jins-0.1]);
    col("gold","#d8b040") for (i=[0:3]) translate([-3.75+i*2.5, jh-1, jl+0.6]) {
        if (bent) { cube([0.6, 0.6, jpin-0.6]); translate([0,0,jpin-1.2]) cube([0.6, jpin_up+1, 0.6]); }
        else cube([0.6, 0.6, jpin + jpin_up]);
    }
}

// ===================== output =====================
module maybe_cut() {
    if (cut) difference() { children(); translate([0,-100,-100]) cube([200,200,200]); }
    else children();
}
if (mode == "print") {
    // front face down on the bed
    rotate([180,0,0]) untilted() case_world();
} else {
    rotate([90,0,0]) maybe_cut() {
        col("case","#e9e4da") case_world();
        if (show_module) tilted() glass_module();
        translate([0, yb, z_m - jack_dz]) usb_a(bent=pins_bent);
    }
}
echo(str("SIZE  depth=", -z_m, "  jack mouth z=", z_m, "  table y=", y_tab));
