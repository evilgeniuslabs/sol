/*
   Sol: https://github.com/evilgeniuslabs/sol
   Copyright (C) 2014-2016 Jason Coon, Evil Genius Labs

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

void drawHand(uint16_t angle, uint8_t endRing, CRGB color)
{
  // rotate 90 degrees counter-clockwise
  angle += 270;

  while(angle >= 360)
    angle -= 360;

  uint8_t index = 0;

  for(uint8_t ringIndex = 0; ringIndex <= endRing; ringIndex++)
  {
    index = angleToPixel360(angle, ringIndex);
    
    leds[index] |= color;
  }
}

void drawHandArc(uint16_t endAngle, uint8_t ringIndex, CRGB color)
{
  static const uint16_t startAngle = 270;
  
  // rotate 90 degrees counter-clockwise
  endAngle += 270;

  while(endAngle >= 360)
    endAngle -= 360;

  uint8_t startIndex = angleToPixel360(startAngle, ringIndex);
  uint8_t endIndex = angleToPixel360(endAngle, ringIndex);
  if(startIndex == endIndex)
  {
    leds[endIndex] = color;
  }
  else
  {
    fillRing360(ringIndex, color, startAngle, endAngle);
  }
}

uint16_t hourAngle = 0;
uint16_t minuteAngle = 0;
uint16_t secondAngle = 0;

void updateClock()
{
  hourAngle = hour();
  if(hourAngle >= 12)
    hourAngle -= 12;
  hourAngle *= 30;

  minuteAngle = minute() * 6;
  secondAngle = second() * 6;
}

void updateFastClock()
{
  secondAngle++;
  if(secondAngle >= 360)
  {
    secondAngle -= 360;
    minuteAngle++;

    if(minuteAngle >= 360)
    {
      minuteAngle -= 360;
      hourAngle++;

      if(hourAngle >= 360)
        hourAngle -= 360;
    }
  }
}

uint8_t handClock()
{
  dimAll(240);
  // fill_solid(leds, NUM_LEDS, CRGB::Black);

  // hour
  drawHand(hourAngle, 5, CRGB::Red);
  
  // minute
  drawHand(minuteAngle, 7, CRGB::Green);
  
  // second
  drawHand(secondAngle, 9, CRGB::Blue);

  updateClock();
//  updateFastClock();
  
  return 8;
}

uint8_t arcClock()
{
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  // hour
  drawHandArc(hourAngle, 7, CRGB::Red);
  
  // minute
  drawHandArc(minuteAngle, 8, CRGB::Green);
  
  // second
  drawHandArc(secondAngle, 9, CRGB::Blue);

  updateClock();
//  updateFastClock();
  
  return 0;
}

