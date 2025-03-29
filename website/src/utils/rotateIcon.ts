type RotateIconOptions = {
  url: string;
  width: number;
  height: number;
};

class RotateIcon {
  options: RotateIconOptions;
  rImg: HTMLImageElement;
  context: CanvasRenderingContext2D | null;
  canvas: HTMLCanvasElement;

  constructor(options: RotateIconOptions) {
    this.options = options;
    this.rImg = new Image();
    this.rImg.src = this.options.url;
    const canvasElement = document.createElement("canvas");
    canvasElement.width = this.options.width;
    canvasElement.height = this.options.height;
    this.canvas = canvasElement;
    this.context = canvasElement.getContext("2d");
  }

  setRotation(options: { deg: number }) {
    if (this.context === null) {
      console.warn("Context was null when rotation was applied");
      return this;
    }
    const canvas = this.context;
    const angle = (options.deg * Math.PI) / 180;
    const centerX = this.options.width / 2;
    const centerY = this.options.height / 2;

    canvas.clearRect(0, 0, this.options.width, this.options.height);
    canvas.save();
    canvas.translate(centerX, centerY);
    canvas.rotate(angle);
    canvas.translate(-centerX, -centerY);
    canvas.drawImage(this.rImg, 0, 0);
    canvas.restore();

    return this;
  }

  getUrl() {
    return this.canvas.toDataURL("image/png");
  }
}

export { RotateIcon };
