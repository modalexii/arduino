// =====================================================================
//  Round AMOLED clock case  -  design "C2", rev 3: USB-C breakout power
//  Board: Waveshare ESP32-S3-Touch-AMOLED-1.43 (46 mm round glass)
//  Power: 22 x 13 mm USB-C breakout PCB (6-pin: VBUS GND CC1 D- D+ CC2).
//         It slides in from the FRONT (before the screen) along rails behind
//         the cavity, its USB-C receptacle ends flush with the back face, and
//         two spring fingers click into its mounting holes so the plug can't
//         push it forward. Unplugging pulls it against the back wall.
//  One piece, no screws. PLA, 0.4 nozzle. See CASE_NOTES.md.
//  (USB-A version: clock_case_usba.scad)
//
//  mode = "print"    -> the case only, front face on the bed. Export to STL.
//  mode = "assembly" -> standing on the table with board + breakout (renders).
// =====================================================================

mode = "print";
part = "all";        // assembly colour group: case glass frame pcb gold black silver white red
cut  = false;        // assembly: remove the +X half (section view)
cut_x = 0;           // assembly: section plane position
show_module = true;  // assembly: draw the screen module
board_dz    = 0;     // assembly: breakout pulled this far toward the front (insertion view)

$fn = 160;

// ---------------- screen module (CHECK) ----------------
glass_d   = 46.0;   // front glass OD                      (Waveshare drawing)
glass_t   = 1.8;    // glass + panel thickness            (drawing: 0.70 + 1.10)
mod_d     = 44.0;   // PCB/frame OD behind the glass       ** MEASURE ** (photo est.)
pcb_back  = 6.2;    // glass front -> PCB back            (drawing)
stack     = 10.2;   // glass front -> standoff tips (6.2 + H4 standoffs)
// standoff centres, front view, board turned so USB-C is on the LEFT  ** MEASURE **
standoffs = [[11.6, -15.75], [11.6, 15.75], [-11.5, -16.25], [-11.5, 16.25]];

// ---------------- USB-C breakout (listing says +-1-2 mm: MEASURE) ----------------
bw    = 22.0;   // PCB width (across the plug axis)
bl    = 13.0;   // PCB length (along the plug axis)
bt    = 1.6;    // PCB thickness
c_w   = 8.94;   // receptacle shell width  (USB-C standard 8.94)
c_h   = 3.26;   // receptacle shell height (USB-C standard 3.16-3.26)
c_l   = 7.35;   // receptacle length
c_over= 1.0;    // receptacle sticks out past the PCB edge (14 - 13 in the listing)
mh_d  = 3.0;    // mounting hole diameter
mh_dx = 16.3;   // mounting hole spacing (listing)
mh_z  = 2.7;    // mounting hole centre from the receptacle-end PCB edge

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
pad_d    = 5.0;                // standoff stop pad
pushout_d= 2.4;                // push-out holes behind the stop pads
chamf_o  = 0.3;                // outer front-rim chamfer
chamf_i  = 0.2;                // inner front-rim chamfer
s_w      = 0.3;                // rail clearance per side, PCB width
s_t      = 0.1;                // rail clearance per side, PCB thickness
c_fit    = 0.35;               // receptacle opening clearance per side (room for a sloppy board)
c_r      = 0.8;                // receptacle opening corner radius
f_t      = 1.0;                // spring finger thickness
f_w      = 2.0;                // spring finger width
f_gap    = 0.8;                // room for the finger to flex
f_bump   = 0.6;                // detent bump height (into the mounting hole)

// ---------------- stance ----------------
tilt     = 12;                 // screen leans back
chin     = 0.5;                // front rim height above the table
cav_clip = 0.4;                // cavity floor height above the rim pivot
y_tab    = -R - chin;          // table plane (world)

// ---------------- breakout placement (puck frame) ----------------
// The board rides just above the two lower stop pads so it can slide straight
// in through the open front, receptacle side DOWN (it hangs between the pads).
pad_top = max([for (p = standoffs) if (p[1] < 0) p[1] + pad_d/2]);
y0      = pad_top + 0.85;            // PCB bottom (receptacle side)
pt      = y0 + bt;                   // PCB top (wire side)
Z_f     = ceil_z;                    // PCB front edge (header holes) = cavity back
Z_b     = Z_f - bl;                  // PCB back edge
Z_out   = Z_b - c_over;              // back face of the case = receptacle face
Z_hole  = Z_b + mh_z;
Z_anch  = Z_f - 2.5;                 // finger root (behind the header pads)
Z_free  = Z_hole - 1.2;              // finger tip
y_top   = pt + s_t + f_t + f_gap + 1.2;   // tail top
tail_hw = 14.5;                      // tail half width

// world <- puck frame (rim pivot at y = -R, z = 0)
module tilted()   { translate([0,-R,0]) rotate([-tilt,0,0]) translate([0,R,0]) children(); }
module untilted() { translate([0,-R,0]) rotate([ tilt,0,0]) translate([0,R,0]) children(); }
module col(name, hex) { if (part=="all") color(hex) children(); else if (part==name) children(); }

// ===================== case =====================
module body_p() {   // puck frame
    translate([0,0,-puck_D]) cylinder(r=R, h=puck_D);
    // tail: the screen's own cylinder carried straight back, below the board
    intersection() {
        translate([0,0,Z_out]) cylinder(r=R-0.2, h=-Z_out-puck_D+0.5);
        translate([-tail_hw, -50, Z_out-1]) cube([2*tail_hw, 50+y_top, 100]);
    }
}

module stop_corbel(p) {   // flat pad at the standoff tip, joined to the side wall
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
        untilted() translate([-60, -R+cav_clip, -100]) cube([120, 120, 200]);   // keep a floor
    }
    translate([0,0,-chamf_i]) cylinder(r1=bore_r, r2=bore_r+chamf_i+0.01, h=chamf_i+0.01);
    translate([0,0,-0.001]) cylinder(r=bore_r+chamf_i+0.01, h=1);
}

module front_chamfer() {
    translate([0,0,-chamf_o-0.1]) difference() {
        cylinder(r=R+1, h=chamf_o+1.1);
        cylinder(r1=R+0.1, r2=R-chamf_o, h=chamf_o+0.1+0.001);
    }
}

module pushout_holes() {
    // lower holes shifted 0.8 mm outward (still on the standoff) to clear the board pocket
    for (p = standoffs) {
        dx = p[1] < 0 ? sign(p[0])*0.8 : 0;
        translate([p[0]+dx, p[1], -100]) cylinder(d=p[1] < 0 ? 2.0 : pushout_d, h=100-stack+0.01, $fn=24);
    }
}

module finger(fx) {   // spring finger, root at the front, detent bump at the tip
    translate([fx-f_w/2, pt+s_t, Z_free]) cube([f_w, f_t, Z_anch-Z_free+0.6]);
    // bump: steep face toward the back, 45-degree ramp toward the front
    hull() {
        translate([fx-0.8, pt+s_t-f_bump, Z_hole-0.9]) cube([1.6, f_bump+0.01, 0.2]);
        translate([fx-0.8, pt+s_t-0.01,  Z_hole-0.9]) cube([1.6, 0.02, 0.2+f_bump+0.6]);
    }
}

module pocket_void() {
    // PCB rails
    translate([-bw/2-s_w, y0-s_t, Z_b]) cube([bw+2*s_w, bt+2*s_t, Z_f-Z_b+0.01]);
    // receptacle / parts side (below the PCB), inside the rails
    translate([-10, y0-c_h-0.3, Z_b]) cube([20, c_h+0.3+0.01, Z_f-Z_b+0.01]);
    // wire side (above the PCB): solder joints + wires running forward
    translate([-7.2, pt, Z_b]) cube([14.4, 1.6, Z_f-Z_b+0.01]);
    // finger slots (finger + flex room)
    for (fx=[-mh_dx/2, mh_dx/2])
        translate([fx-f_w/2-0.4, pt, Z_b]) cube([f_w+0.8, s_t+f_t+f_gap, Z_anch-Z_b]);
    // receptacle opening through the back wall: rounded rectangle centred on the
    // receptacle, with a small rounded lead-in on the outside
    translate([0, y0-c_h/2, Z_out-1]) rrect(c_w+2*c_fit, c_h+2*c_fit, c_r, Z_b-Z_out+1.01);
    hull() {
        translate([0, y0-c_h/2, Z_out-0.01]) rrect(c_w+2*c_fit+0.6, c_h+2*c_fit+0.6, c_r+0.3, 0.01);
        translate([0, y0-c_h/2, Z_out+0.3])  rrect(c_w+2*c_fit, c_h+2*c_fit, c_r, 0.01);
    }
}

// rounded rectangle centred on x/y, extruded along +z
module rrect(w, h, r, len) {
    linear_extrude(height=len) offset(r=r) square([w-2*r, h-2*r], center=true, $fn=48);
}

// release holes: from the (hidden) underside straight up to the mounting holes.
// Push a paperclip up through each one to lift that finger's bump out of the
// PCB hole, then slide the board out the front.
module release_holes() {
    for (fx=[-mh_dx/2, mh_dx/2]) translate([fx, -60, Z_hole]) rotate([-90,0,0]) cylinder(d=1.6, h=60+y0-1, $fn=16);
}

module case_p() {   // puck frame
    difference() {
        body_p();
        cavity(); pushout_holes(); front_chamfer(); release_holes();
        difference() { pocket_void(); for (fx=[-mh_dx/2, mh_dx/2]) finger(fx); }
    }
}
module case_world() {
    difference() {
        tilted() case_p();
        translate([-100, y_tab-100, -100]) cube([200, 100, 200]);   // table
    }
}

// ===================== preview parts (puck frame) =====================
module glass_module() {
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
module usbc_board() {   // seated position
    col("red","#c8252c") difference() {
        translate([-bw/2, y0, Z_b]) cube([bw, bt, bl]);
        for (fx=[-mh_dx/2, mh_dx/2]) translate([fx, y0-1, Z_hole]) rotate([-90,0,0]) cylinder(d=mh_d, h=bt+2, $fn=32);
        for (i=[0:5]) translate([-6.35+i*2.54, y0-1, Z_f-1.3]) rotate([-90,0,0]) cylinder(d=1.0, h=bt+2, $fn=16);
    }
    col("silver","#c8ccd0") difference() {
        translate([-c_w/2, y0-c_h, Z_out]) cube([c_w, c_h, c_l]);
        translate([-c_w/2+0.5, y0-c_h+0.5, Z_out-0.01]) cube([c_w-1, c_h-1, c_l-1.5]);
    }
    col("black","#151515") translate([-3.3, y0-c_h/2-0.35, Z_out+0.3]) cube([6.6, 0.7, c_l-2]);
    col("gold","#d8b040") for (i=[0:5]) translate([-6.35+i*2.54, pt, Z_f-1.3]) cylinder(d=1.7, h=0.4, $fn=16);
    col("black","#202020") for (x=[-9.05, 7.9]) translate([x-0.8, y0-0.45, Z_f-5.5]) cube([1.6, 0.45, 0.8]);
}

// ===================== output =====================
module maybe_cut() {
    if (cut) difference() { children(); translate([cut_x,-100,-100]) cube([200,200,200]); }
    else children();
}
if (mode == "print") {
    rotate([180,0,0]) untilted() case_world();   // front face down on the bed
} else {
    rotate([90,0,0]) maybe_cut() {
        col("case","#e9e4da") case_world();
        tilted() {
            if (show_module) glass_module();
            translate([0,0,board_dz]) usbc_board();
        }
    }
}
echo(str("y0=", y0, " Z_out=", Z_out, " y_top=", y_top));
