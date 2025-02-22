const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');

const ratio = 16 / 9;
const zoom = 1;

// const dpi = window.devicePixelRatio;
// const resolution = Math.max(screen.width, screen.height) * dpi * zoom;
// const width = resolution * ratio;
// const height = resolution;
const width = 1600;
const height = 900;

canvas.width = width;
canvas.style.width = `${width}px`;
canvas.height = height;
canvas.style.height = `${height}px`;

window.onresize = () => {
    canvas.style.transform = `scale(${Math.min(window.innerWidth / width, window.innerHeight / height)})`;
    canvas.style.left = `${(window.innerWidth - width) / 2}px`;
    canvas.style.top = `${(window.innerHeight - height) / 2}px`;
};

window.onresize();

canvas.oncontextmenu = (e) => {
    return e.preventDefault();
}