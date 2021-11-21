var Plate_RO;
var Plate;


if (typeof ResizeObserver != "undefined")
{
  Plate_RO = new ResizeObserver(entries =>
  {
    for (let entry of entries)
    {
      if (entry.devicePixelContentBoxSize)
      {
        const dev_size = entry.devicePixelContentBoxSize;
        Plate.f_canvas_resize_exact(entry.target.id, dev_size[0].inlineSize, dev_size[0].blockSize);
      }
      else
        Plate.f_canvas_resize(entry.target.id);
    }
  });
}
else
{
  console.log("info: no resize observer");
}

plate().then(module =>
{
  Plate = module;

  if (typeof initPlate == 'function')
  {
    initPlate();

    window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', event =>
    {
      Plate.f_color_mode_change(event.matches ? 1 : 0);
    });

    window.addEventListener('hashchange', function(event)
    {
      event.preventDefault();

      Plate.f_fragment_change(window.location.hash);
    });
  }
});

