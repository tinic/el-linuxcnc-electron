import { RendererPublicInterface } from "troisjs";
import * as THREE from "three";
import { Line2, LineGeometry, LineMaterial } from "three-fatline";

export default class Backplot {
  renderer: RendererPublicInterface;

  json: any;

  xmin: number;
  ymin: number;
  zmin: number;
  xmax: number;
  ymax: number;
  zmax: number;

  fmin: number = +999999;
  fmax: number = -999999;

  lmin: number = +999999;
  lmax: number = -999999;

  tlin: number = 0;

  xoff: number;
  yoff: number;
  zoff: number;
  scal: number;
  omat: THREE.Matrix4 = new THREE.Matrix4();

  pruningMinLength: number = 0.005;

  boundingBoxMaterial: LineMaterial = new LineMaterial({
    color: 0xaaaaaa,
    linewidth: 1,
    resolution: new THREE.Vector2(800, 600),
  });

  backplotMaterial: LineMaterial = new LineMaterial({
    color: 0xffffff,
    linewidth: 1,
    resolution: new THREE.Vector2(800, 600),
  });

  backplotMaterial0: Array<LineMaterial> = new Array<LineMaterial>();
  backplotMaterial1: Array<LineMaterial> = new Array<LineMaterial>();
  backplotMaterial2: LineMaterial;
  backplotMaterial3: LineMaterial;

  constructor(_json: any, _renderer: RendererPublicInterface) {
    this.json = _json;
    this.renderer = _renderer;

    let color0 = new THREE.Color(0xff0000);
    let color1 = new THREE.Color(0x0000ff);

    this.backplotMaterial2 = new LineMaterial({
      color: 0xff00ff,
      linewidth: 1,
      resolution: new THREE.Vector2(800, 600),
    });

    this.backplotMaterial3 = new LineMaterial({
      color: 0xff00ff,
      linewidth: 3,
      resolution: new THREE.Vector2(800, 600),
    });

    for (let c = 0; c < 256; c++) {
      this.backplotMaterial0[c] = new LineMaterial({
        color: color0.lerpHSL(color1, c / 32768).getHex(),
        linewidth: 1,
        resolution: new THREE.Vector2(800, 600),
      });

      this.backplotMaterial1[c] = new LineMaterial({
        color: color0.lerpHSL(color1, c / 32768).getHex(),
        linewidth: 4,
        resolution: new THREE.Vector2(800, 600),
      });
    }

    this.xmin = this.json["extents"][0];
    this.ymin = this.json["extents"][1];
    this.zmin = this.json["extents"][2];
    this.xmax = this.json["extents"][3];
    this.ymax = this.json["extents"][4];
    this.zmax = this.json["extents"][5];

    this.xoff = -(this.xmax - this.xmin) / 2 - this.xmin;
    this.yoff = -(this.ymax - this.ymin) / 2 - this.ymin;
    this.zoff = -(this.zmax - this.zmin) / 2 - this.zmin;

    let maxx = Math.abs(this.xmax - this.xmin);
    let maxy = Math.abs(this.ymax - this.ymin);
    let maxz = Math.abs(this.zmax - this.zmin);

    this.scal = 1.0 / Math.max(maxz, Math.max(maxx, maxy));

    let smat = new THREE.Matrix4().makeScale(this.scal, this.scal, this.scal);
    let tmat = new THREE.Matrix4().makeTranslation(
      this.xoff,
      this.yoff,
      this.zoff
    );

    this.omat.identity();
    this.omat.multiply(smat);
    this.omat.multiply(tmat);

    this.xmin += this.xoff - 0.01;
    this.xmax += this.xoff + 0.01;
    this.ymin += this.yoff - 0.01;
    this.ymax += this.yoff + 0.01;
    this.zmin += this.zoff - 0.01;
    this.zmax += this.zoff + 0.01;

    this.xmin *= this.scal;
    this.xmax *= this.scal;
    this.ymin *= this.scal;
    this.ymax *= this.scal;
    this.zmin *= this.scal;
    this.zmax *= this.scal;

    this.pruneShortSegments();
    this.createLine2Geometry();
  }

  pruneShortSegments() {
    for (let entry of this.json["backplot"]) {
      let entryType = entry["type"];
      switch (entryType) {
        case "arcfeed":
        case "feed":
        case "trav":
          let pruned = [];
          let pruning = false;
          let pruningIndex: number = 0;
          let lines = entry[entryType];
          for (let c = 0; c < lines.length; c++) {
            let line = lines[c];
            let l0 = new THREE.Vector3(
              pruning ? lines[pruningIndex]["coords"][0] : line["coords"][0],
              pruning ? lines[pruningIndex]["coords"][1] : line["coords"][1],
              pruning ? lines[pruningIndex]["coords"][2] : line["coords"][2]
            ).applyMatrix4(this.omat);
            let l1 = new THREE.Vector3(
              line["coords"][3],
              line["coords"][4],
              line["coords"][5]
            ).applyMatrix4(this.omat);
            if (
              l0.distanceTo(l1) < this.pruningMinLength &&
              c != lines.length - 1
            ) {
              if (!pruning) {
                pruning = true;
                pruningIndex = c;
              }
            } else {
              if (pruning) {
                pruning = false;
                line["coords"][0] = lines[pruningIndex]["coords"][0];
                line["coords"][1] = lines[pruningIndex]["coords"][1];
                line["coords"][2] = lines[pruningIndex]["coords"][2];
              }
              pruned.push(line);
            }
          }
          entry[entryType] = pruned;
          break;
        case "dwell":
          break;
      }
    }
  }

  createLine2Geometry() {
    for (let entry of this.json["backplot"]) {
      let entryType = entry["type"];
      if (entry.hasOwnProperty("line")) {
        this.lmin = Math.min(this.lmin, entry["line"]);
        this.lmax = Math.max(this.lmax, entry["line"]);
      }
      switch (entryType) {
        case "arcfeed":
        case "feed":
        case "trav":
          for (let line of entry[entryType]) {
            if (entryType == "feed" || entryType == "arcfeed") {
              if (line.hasOwnProperty("rate")) {
                this.fmin = Math.min(this.fmin, line["rate"]);
                this.fmax = Math.max(this.fmax, line["rate"]);
              }
            }
            let points: Array<number> = [];
            let l0 = new THREE.Vector3(
              line["coords"][0],
              line["coords"][1],
              line["coords"][2]
            ).applyMatrix4(this.omat);
            let l1 = new THREE.Vector3(
              line["coords"][3],
              line["coords"][4],
              line["coords"][5]
            ).applyMatrix4(this.omat);
            points.push(l0.x, l0.y, l0.z, l1.x, l1.y, l1.z);
            line["points"] = points;
            var geometry: LineGeometry = new LineGeometry();
            geometry.setPositions(points);
            line["geometry"] = geometry;
            let line2 = new Line2(geometry, this.backplotMaterial);
            line2.computeLineDistances();
            line["line2"] = line2;
            this.tlin++;
          }
          break;
        case "dwell":
          console.log("dwell not implemented!");
          break;
      }
    }
  }

  addBackplotToScene() {
    for (let entry of this.json["backplot"]) {
      let entryType = entry["type"];
      switch (entryType) {
        case "arcfeed":
        case "feed":
        case "trav":
          for (let line of entry[entryType]) {
            if (line.hasOwnProperty("line2")) {
              let line2 = line["line2"] as Line2;
              line2.computeLineDistances();
              this.renderer.scene?.add(line2);
            }
          }
          break;
        case "dwell":
          break;
      }
    }
  }

  addBoundingBoxToScene() {
    let geom_lines: Array<Array<number>> = [];
    geom_lines.push([
      this.xmin,
      this.ymin,
      this.zmin,
      this.xmax,
      this.ymin,
      this.zmin,
    ]);
    geom_lines.push([
      this.xmax,
      this.ymin,
      this.zmin,
      this.xmax,
      this.ymax,
      this.zmin,
    ]);
    geom_lines.push([
      this.xmax,
      this.ymax,
      this.zmin,
      this.xmin,
      this.ymax,
      this.zmin,
    ]);
    geom_lines.push([
      this.xmin,
      this.ymax,
      this.zmin,
      this.xmin,
      this.ymin,
      this.zmin,
    ]);
    geom_lines.push([
      this.xmin,
      this.ymin,
      this.zmax,
      this.xmax,
      this.ymin,
      this.zmax,
    ]);
    geom_lines.push([
      this.xmax,
      this.ymin,
      this.zmax,
      this.xmax,
      this.ymax,
      this.zmax,
    ]);
    geom_lines.push([
      this.xmax,
      this.ymax,
      this.zmax,
      this.xmin,
      this.ymax,
      this.zmax,
    ]);
    geom_lines.push([
      this.xmin,
      this.ymax,
      this.zmax,
      this.xmin,
      this.ymin,
      this.zmax,
    ]);
    geom_lines.push([
      this.xmin,
      this.ymin,
      this.zmin,
      this.xmin,
      this.ymin,
      this.zmax,
    ]);
    geom_lines.push([
      this.xmax,
      this.ymin,
      this.zmin,
      this.xmax,
      this.ymin,
      this.zmax,
    ]);
    geom_lines.push([
      this.xmax,
      this.ymax,
      this.zmin,
      this.xmax,
      this.ymax,
      this.zmax,
    ]);
    geom_lines.push([
      this.xmin,
      this.ymax,
      this.zmin,
      this.xmin,
      this.ymax,
      this.zmax,
    ]);
    for (let geom_line of geom_lines) {
      var geometry = new LineGeometry();
      geometry.setPositions(geom_line);
      let line = new Line2(geometry, this.boundingBoxMaterial);
      line.computeLineDistances();
      this.renderer.scene?.add(line);
    }
  }

  updateProgress(lineidx: number) {
    let clin: number = 0;
    for (let entry of this.json["backplot"]) {
      let entryType = entry["type"];
      switch (entryType) {
        case "arcfeed":
        case "feed":
        case "trav":
          for (let line of entry[entryType]) {
            let line2 = line["line2"] as Line2;
            if (
              (entryType == "feed" || entryType == "arcfeed") &&
              line.hasOwnProperty("rate")
            ) {
              let r = Math.floor(((clin / this.tlin) * 256) % 256);
              line2.material =
                clin > lineidx
                  ? this.backplotMaterial0[r]
                  : this.backplotMaterial1[r];
            } else {
              line2.material =
                clin > lineidx
                  ? this.backplotMaterial2
                  : this.backplotMaterial3;
            }
            clin++;
          }
          break;
        case "dwell":
          break;
      }
    }
  }
}
