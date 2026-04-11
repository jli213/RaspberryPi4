# Adapted from https://coderslegacy.com/python/pygame-platformer-game-development/

from os import environ
environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'

import pygame
from pygame.locals import *
import sys
import random
import time
import serial
 
pygame.init()
vec = pygame.math.Vector2
 
HEIGHT = 450
WIDTH = 400
ACC = 0.5
FRIC = -0.12
FPS = 60
 
FramePerSec = pygame.time.Clock()
 
displaysurface = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Game")
 
class Player(pygame.sprite.Sprite):
    def __init__(self):
        super().__init__() 
        #self.image = pygame.image.load("character.png")
        self.surf = pygame.Surface((30, 30))
        self.surf.fill((255,255,0))
        self.rect = self.surf.get_rect()
   
        self.pos = vec((30, 360))
        self.vel = vec(0,0)
        self.acc = vec(0,0)
        self.jumping = False
        self.score = 0 
 
    def set_acc(self, acc):
        self.acc = vec(0,0.5)
        if acc is not None:
            self.acc.x = acc

    def move(self):
        self.acc.x += self.vel.x * FRIC
        self.vel += self.acc
        self.pos += self.vel + 0.5 * self.acc
         
        if self.pos.x > WIDTH:
            self.pos.x = 0
        if self.pos.x < 0:
            self.pos.x = WIDTH
             
        self.rect.midbottom = self.pos
 
    def jump(self): 
        hits = pygame.sprite.spritecollide(self, platforms, False)
        if hits and not self.jumping:
           self.jumping = True
           self.vel.y = -15
 
    def cancel_jump(self):
        if self.jumping:
            if self.vel.y < -3:
                self.vel.y = -3
 
    def update(self):
        hits = pygame.sprite.spritecollide(self ,platforms, False)
        if self.vel.y > 0:        
            if hits:
                if self.pos.y < hits[0].rect.bottom:
                    if hits[0].point == True:   
                        hits[0].point = False   
                        self.score += 1          
                    self.pos.y = hits[0].rect.top +1
                    self.vel.y = 0
                    self.jumping = False
 
 
class platform(pygame.sprite.Sprite):
    def __init__(self):
        super().__init__()
        self.surf = pygame.Surface((random.randint(50,100), 12))
        self.surf.fill((0,255,0))
        self.rect = self.surf.get_rect(center = (random.randint(0,WIDTH-10),
                                                 random.randint(0, HEIGHT-30)))
        self.speed = random.randint(-1, 1)
        
        self.point = True   
        self.moving = True
        
    
    def move(self):
        if self.moving == True:  
            self.rect.move_ip(self.speed,0)
            if self.speed > 0 and self.rect.left > WIDTH:
                self.rect.right = 0
            if self.speed < 0 and self.rect.right < 0:
                self.rect.left = WIDTH
 
 
def check(platform, groupies):
    if pygame.sprite.spritecollideany(platform,groupies):
        return True
    else:
        for entity in groupies:
            if entity == platform:
                continue
            if (abs(platform.rect.top - entity.rect.bottom) < 40) and (abs(platform.rect.bottom - entity.rect.top) < 40):
                return True
        C = False
 
def plat_gen():
    while len(platforms) < 6:
        width = random.randrange(50,100)
        p  = platform()      
        C = True
         
        while C:
             p = platform()
             p.rect.center = (random.randrange(0, WIDTH - width),
                              random.randrange(-50, 0))
             C = check(p, platforms)
        platforms.add(p)
        all_sprites.add(p)
 

def makePT1():
    PT1 = platform()

    PT1.surf = pygame.Surface((WIDTH, 20))
    PT1.surf.fill((255,0,0))
    PT1.rect = PT1.surf.get_rect(center = (WIDTH/2, HEIGHT - 10))

    PT1.moving = False
    PT1.point = False   ##

    all_sprites.add(PT1)
    platforms.add(PT1)



def EVENT_HANDLER_KEYBOARD():
    for event in pygame.event.get():
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_SPACE:
                P1.jump()
        if event.type == pygame.KEYUP:
            if event.key == pygame.K_SPACE:
                P1.cancel_jump()

    pressed_keys = pygame.key.get_pressed()
    if pressed_keys[K_LEFT]:
        P1.set_acc(-ACC)
    elif pressed_keys[K_RIGHT]:
        P1.set_acc(ACC)
    else:
        P1.set_acc(0)





def EVENT_HANDLER_UART():

    # Real-time UART data coming in. Empty if no data (non-blocking).
    # ".decode(...)" turns data into Python string.
    # (https://pyserial.readthedocs.io/en/latest/pyserial_api.html#serial.Serial.read_all)
    out = ser.read_all().decode('utf-8')

    # Your code goes here:

    if out == "":
        return

    # Split into lines in case multiple arrive at once
    lines = out.strip().split("\n")

    for line in lines:
        line = line.strip()
        if not (line.startswith("(") and line.endswith(")")):
            continue

        # Remove parentheses
        line = line.strip("()")

        # Split into two numbers
        try:
            v1_str, btn_str = line.split()
            v1 = int(v1_str)
            buttons = int(btn_str)
        except:
            continue

        # --- Movement ---
        if v1 < 505:
            P1.set_acc(ACC)
        elif v1 > 520:
            P1.set_acc(-ACC)
        else:
            P1.set_acc(0) # around the 512 (allow for about 10 units of discrepency)

        # --- Jump (Button B = bit 1 = value 2) ---
        if buttons & 0x02:
            P1.jump()
        else:
            P1.cancel_jump()


    ...




def main():
    global all_sprites, platforms, P1, ser

    ser = None
    try:
        ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=1)
    except serial.serialutil.SerialException:
        print("Can't connect to serial!")

    P1 = Player()

    all_sprites = pygame.sprite.Group()
    all_sprites.add(P1)

    platforms = pygame.sprite.Group()

    makePT1()

    for x in range(random.randint(4,5)):
        C = True
        pl = platform()
        while C:
            pl = platform()
            C = check(pl, platforms)
        platforms.add(pl)
        all_sprites.add(pl)

    while True:
        P1.update()

        #EVENT_HANDLER_KEYBOARD()
        EVENT_HANDLER_UART()

        # game over condition
        if P1.rect.top > HEIGHT:
            time.sleep(1)
            displaysurface.fill((255,0,0))
            f = pygame.font.SysFont("Verdana", 20)
            g  = f.render("Game Over", True, (0,0,0))
            displaysurface.blit(g, (WIDTH/2 - 40, 50))
            pygame.display.update()
            time.sleep(1)
            P1.score = 0
            P1.pos = vec((30, 360))
            makePT1()
            pygame.display.update()

        # delete old platforms
        if P1.rect.top <= HEIGHT / 3:
            P1.pos.y += abs(P1.vel.y)
            for plat in platforms:
                plat.rect.y += abs(P1.vel.y)
                if plat.rect.top >= HEIGHT:
                    plat.kill()

        plat_gen()
        displaysurface.fill((0,0,0))
        f = pygame.font.SysFont("Verdana", 20)
        g  = f.render(str(P1.score), True, (123,255,0))
        displaysurface.blit(g, (WIDTH/2, 10))

        for entity in all_sprites:
            displaysurface.blit(entity.surf, entity.rect)
            entity.move()

        pygame.display.update()
        FramePerSec.tick(FPS)


if __name__ == '__main__':
    main()
