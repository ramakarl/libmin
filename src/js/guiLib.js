


function giLib ( canvas, context )
{
  this.ctx = context;
  this.cvs = canvas;
  this.width = canvas.width;
  this.height = canvas.height;
  
  this.root = 0;

  this.view = new Object();
  this.view.cx = 0;
  this.view.cy = 0;
  this.view.x1 = 0;
  this.view.y1 = 0;
  this.view.x2 = this.width;
  this.view.y2 = this.height;  
  this.zoom = 1.0;                 // initial zoom
  
  this.setView ( 0, 0, this.width, this.height, 1.0 );    // set view for first time
  
  this.dirty_flag = true;
  this.default_line_width = 5;
  this.default_stroke_color = "rgb( 0, 160, 0 )";
  this.default_fill_color = "rgb( 0, 160, 0 )";
}

giLib.prototype.setRoot = function( obj )
{
  this.root = obj;
}

giLib.prototype.setView = function( x1, y1, x2, y2, z )
{
  // set center and zoom
  this.view.cx = (x1+x2)/2;
  this.view.cy = (y1+y2)/2;
  this.zoom = z;

  // compute the view region
  var dx = (x2-x1) / (z * 2.0);
  var dy = (y2-y1) / (z * 2.0);
  this.view.x1 = this.view.cx - dx;
  this.view.y1 = this.view.cy - dy;
  this.view.x2 = this.view.cx + dx;
  this.view.y2 = this.view.cy + dy;

  // update the transform
  this.transform = [
            [ z, 0, -this.view.x1*z ],
            [ 0, z, -this.view.y1*z ],
            [ 0, 0, 1 ] ];
}

giLib.prototype.devToWorld = function(x, y)
{
  var wx = this.view.x1 + x * ( this.view.x2 - this.view.x1) / this.width;
  var wy = this.view.y1 + y * ( this.view.y2 - this.view.y1) / this.height;
  return { "x" : wx, "y" : wy};
}

giLib.prototype.render = function()
{
  // clear view
  this.ctx.setTransform ( 1, 0, 0, 1, 0, 0 );
  this.ctx.clearRect( 0, 0, this.width, this.height );
  
  // setup world space (once, by view transform)
  M = this.transform;  
  this.ctx.setTransform( M[0][0], M[1][0], M[0][1], M[1][1], M[0][2], M[1][2] );

  // render all from root
  this.root.draw ();
}