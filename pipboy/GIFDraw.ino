// Draw a line of image directly on the LCD
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > w)
  {
    iWidth = w - pDraw->iX;
  }
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= h || pDraw->iX >= w || iWidth < 1)
  {
    return;
  }
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
      {
        s[x] = pDraw->ucBackground;
      }
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      }           // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        gfx->draw16bitBeRGBBitmap(pDraw->iX + x, y, usTemp, iCount, 1);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
        {
          iCount++;
        }
        else
        {
          s--;
        }
      }
      if (iCount)
      {
        x += iCount; // skip these
        iCount = 0;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x = 0; x < iWidth; x++)
    {
      usTemp[x] = usPalette[*s++];
    }
    gfx->draw16bitBeRGBBitmap(pDraw->iX + VAULT_BOY_X, y + VAULT_BOY_Y, usTemp, iWidth, 1);
  }
} /* GIFDraw() */