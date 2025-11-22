

function giGrid ()
{
  // All regions have this
  this.x1 = 0;
  this.y1 = 0;
  this.x2 = gi.width;
  this.y2 = gi.height;    
  this.bClip = false;  
  this.bOverlay = false;
  this.visible = true;    
  this.parent = null;
  this.eventCallback = null;
  this.backClr = "rgba(200,200,200,255)";
  this.borderClr = "rgba(50,50,50,255)";
}

giGrid.prototype.setSize = function( x1, y1, x2, y2 )
{
  this.x1 = x1;
  this.y1 = y1;
  this.x2 = x2;
  this.y2 = y2;  
  // this.setWorldMatrix ();
}

giGrid.prototype.setClip = function ( b )
{
  this.bClip = b;
}

giGrid.prototype.OnEvent = function( e )
{
   if ( e.name=="scroll" ) {
      this.scrolly = e.retrieveInt();
      return true;
   }
   return false;
}

giGrid.prototype.setBackClr = function ( r, g, b, a )
{
  this.bgColor = "rgba("+Math.floor(r*255.0)+","+Math.floor(g*255.0)+","+Math.floor(b*255.0)+","+a+")";
}

giGrid.prototype.draw = function()
{
  // background
  gi.ctx.fillStyle = this.backClr;
  gi.ctx.fillRect ( this.x1, this.y1, this.x2, this.y2 );
  
  // border  
  gi.ctx.lineWidth = 1;        
  gi.ctx.strokeStyle = this.borderClr;          
  gi.ctx.strokeRect ( this.x1, this.y1, this.x2, this.y2);   
}
