# Themes

Some themes I made!

## Noir-esque 
<img width="657" height="369" alt="image" src="https://github.com/user-attachments/assets/4ad8b0c0-b399-4ff8-bb44-6d2a79815779" />


```css

/* ========================================
   HyprWave - Noir-esque Theme
   Dark backdrop with cream/gold accents
   Based on retro-futuristic anime aesthetic
   ======================================== */

:root {
    --bg-primary: rgba(25, 25, 25, 0.75);
    --bg-secondary: rgba(30, 30, 30, 0.75);
    --bg-album-cover: rgba(35, 35, 35, 0.85);
    --bg-album-secondary: rgba(40, 40, 40, 0.85);
    --btn-default: rgba(100, 95, 85, 0.85);
    --btn-default-secondary: rgba(90, 85, 75, 0.85);
    --btn-default-hover: rgba(120, 115, 105, 0.92);
    --btn-default-hover-secondary: rgba(110, 105, 95, 0.92);
    --btn-play: rgba(240, 220, 170, 0.95);
    --btn-play-secondary: rgba(230, 210, 160, 0.95);
    --btn-play-hover: rgba(250, 235, 190, 0.98);
    --btn-play-hover-secondary: rgba(245, 225, 180, 0.98);
    --btn-play-active: rgba(220, 200, 150, 0.95);
    --btn-play-active-secondary: rgba(210, 190, 140, 0.95);
    --btn-expand: rgba(255, 170, 80, 0.95);
    --btn-expand-secondary: rgba(245, 160, 70, 0.95);
    --btn-expand-hover: rgba(255, 190, 100, 0.98);
    --btn-expand-hover-secondary: rgba(255, 180, 90, 0.98);
    --btn-expand-active: rgba(235, 150, 60, 0.95);
    --btn-expand-active-secondary: rgba(225, 140, 50, 0.95);
    --progress-bg: rgba(50, 50, 50, 0.25);
    --progress-fill-start: rgba(240, 220, 170, 0.95);
    --progress-fill-end: rgba(255, 170, 80, 0.95);
    --handle-color: rgba(240, 220, 170, 0.98);
    --handle-hover: rgba(250, 235, 190, 1.0);
    --handle-border: rgba(220, 200, 150, 0.5);
    --handle-shadow: rgba(240, 220, 170, 0.5);
    --text-primary: rgba(245, 240, 230, 0.95);
    --text-secondary: rgba(220, 210, 195, 0.85);
    --text-tertiary: rgba(180, 170, 155, 0.75);
    --text-muted: rgba(140, 130, 115, 0.65);
    --border-primary: rgba(100, 95, 85, 0.35);
    --border-button: rgba(90, 85, 75, 0.3);
    --border-button-hover: rgba(120, 115, 105, 0.45);
    --border-play: rgba(240, 220, 170, 0.4);
    --border-play-hover: rgba(250, 235, 190, 0.55);
    --border-expand: rgba(255, 170, 80, 0.4);
    --border-expand-hover: rgba(255, 190, 100, 0.55);
    --shadow-default: rgba(0, 0, 0, 0.4);
    --shadow-button: rgba(0, 0, 0, 0.3);
    --shadow-play: rgba(240, 220, 170, 0.4);
    --shadow-play-hover: rgba(250, 235, 190, 0.6);
    --shadow-expand: rgba(255, 170, 80, 0.4);
    --shadow-expand-hover: rgba(255, 190, 100, 0.6);
    --shadow-focus: rgba(240, 220, 170, 0.6);
    --border-radius-container: 100px;
    --border-radius-section: 20px;
    --border-radius-album: 16px;
    --border-radius-button: 50%;
    --border-radius-progress: 2px;
    --padding-container: 12px;
    --padding-section: 16px;
}

.visualizer-bar {
    background: linear-gradient(180deg, 
        rgba(250, 235, 190, 0.98),
        rgba(255, 190, 100, 0.98));
    border-radius: 0px;
    transition: all 0.05s ease-out;
    margin: 0px;
    min-width: 1px;
    min-height: 3px;
    box-shadow: 0 0 10px rgba(240, 220, 170, 0.5);
}
```

## Emerald Splash
<img width="650" height="366" alt="image" src="https://github.com/user-attachments/assets/bc8fd25d-681c-4111-839a-9ab37df626b6" />

```css
/* ========================================
   HyprWave - Emerald Splash Theme
   Dark forest with bright emerald/mint accents
   Based on mystical forest aesthetic
   ======================================== */

:root {
    /* Background Colors - Deep Forest */
    --bg-primary: rgba(20, 30, 25, 0.75);
    --bg-secondary: rgba(25, 35, 30, 0.75);
    --bg-album-cover: rgba(30, 40, 35, 0.85);
    --bg-album-secondary: rgba(35, 45, 40, 0.85);
    
    /* Button Colors - Default (Prev/Next) - Forest Floor */
    --btn-default: rgba(70, 90, 75, 0.85);
    --btn-default-secondary: rgba(60, 80, 65, 0.85);
    --btn-default-hover: rgba(90, 110, 95, 0.92);
    --btn-default-hover-secondary: rgba(80, 100, 85, 0.92);
    
    /* Button Colors - Play/Pause - Bright Emerald */
    --btn-play: rgba(50, 255, 150, 0.95);
    --btn-play-secondary: rgba(40, 235, 140, 0.95);
    --btn-play-hover: rgba(70, 255, 170, 0.98);
    --btn-play-hover-secondary: rgba(60, 245, 160, 0.98);
    --btn-play-active: rgba(30, 225, 130, 0.95);
    --btn-play-active-secondary: rgba(20, 205, 120, 0.95);
    
    /* Button Colors - Expand - Mint Glow */
    --btn-expand: rgba(100, 255, 200, 0.95);
    --btn-expand-secondary: rgba(90, 245, 190, 0.95);
    --btn-expand-hover: rgba(120, 255, 220, 0.98);
    --btn-expand-hover-secondary: rgba(110, 255, 210, 0.98);
    --btn-expand-active: rgba(80, 235, 180, 0.95);
    --btn-expand-active-secondary: rgba(70, 225, 170, 0.95);
    
    /* Progress Bar Colors */
    --progress-bg: rgba(40, 50, 45, 0.25);
    --progress-fill-start: rgba(50, 255, 150, 0.95);
    --progress-fill-end: rgba(100, 255, 200, 0.95);
    
    /* Slider Handle Colors */
    --handle-color: rgba(50, 255, 150, 0.98);
    --handle-hover: rgba(70, 255, 170, 1.0);
    --handle-border: rgba(30, 225, 130, 0.5);
    --handle-shadow: rgba(50, 255, 150, 0.5);
    
    /* Text Colors - Bright on dark */
    --text-primary: rgba(230, 250, 240, 0.95);
    --text-secondary: rgba(190, 220, 205, 0.85);
    --text-tertiary: rgba(150, 190, 170, 0.75);
    --text-muted: rgba(110, 150, 130, 0.65);
    
    /* Border Colors */
    --border-primary: rgba(70, 90, 75, 0.35);
    --border-button: rgba(60, 80, 65, 0.3);
    --border-button-hover: rgba(90, 110, 95, 0.45);
    --border-play: rgba(50, 255, 150, 0.4);
    --border-play-hover: rgba(70, 255, 170, 0.55);
    --border-expand: rgba(100, 255, 200, 0.4);
    --border-expand-hover: rgba(120, 255, 220, 0.55);
    
    /* Shadow Colors */
    --shadow-default: rgba(0, 0, 0, 0.4);
    --shadow-button: rgba(0, 0, 0, 0.3);
    --shadow-play: rgba(50, 255, 150, 0.5);
    --shadow-play-hover: rgba(70, 255, 170, 0.7);
    --shadow-expand: rgba(100, 255, 200, 0.5);
    --shadow-expand-hover: rgba(120, 255, 220, 0.7);
    --shadow-focus: rgba(50, 255, 150, 0.7);
    
    /* Spacing & Sizes */
    --border-radius-container: 100px;
    --border-radius-section: 20px;
    --border-radius-album: 16px;
    --border-radius-button: 50%;
    --border-radius-progress: 2px;
    
    --padding-container: 12px;
    --padding-section: 16px;
}

/* Visualizer bars - bright emerald to mint gradient */
.visualizer-bar {
    background: linear-gradient(180deg, 
        rgba(70, 255, 170, 0.98),
        rgba(120, 255, 220, 0.98));
    border-radius: 0px;
    transition: all 0.05s ease-out;
    margin: 0px;
    min-width: 1px;
    min-height: 3px;
    box-shadow: 0 0 12px rgba(50, 255, 150, 0.6);
}
```

## Pink Europa

<img width="651" height="363" alt="image" src="https://github.com/user-attachments/assets/77262d51-8916-447c-9c42-30ca425fcd24" />

```css
/* ========================================
   HyprWave - Pink Europa Theme
   Dark silhouettes with pink/peach sunset glow
   Based on lo-fi aesthetic
   ======================================== */

:root {
    /* Background Colors - Evening Sky */
    --bg-primary: rgba(40, 45, 60, 0.75);
    --bg-secondary: rgba(45, 50, 65, 0.75);
    --bg-album-cover: rgba(50, 55, 70, 0.85);
    --bg-album-secondary: rgba(55, 60, 75, 0.85);
    
    /* Button Colors - Default (Prev/Next) - Dusk Gray */
    --btn-default: rgba(90, 85, 100, 0.85);
    --btn-default-secondary: rgba(80, 75, 90, 0.85);
    --btn-default-hover: rgba(110, 105, 120, 0.92);
    --btn-default-hover-secondary: rgba(100, 95, 110, 0.92);
    
    /* Button Colors - Play/Pause - Sunset Pink */
    --btn-play: rgba(255, 150, 180, 0.95);
    --btn-play-secondary: rgba(245, 140, 170, 0.95);
    --btn-play-hover: rgba(255, 170, 195, 0.98);
    --btn-play-hover-secondary: rgba(255, 160, 185, 0.98);
    --btn-play-active: rgba(235, 130, 160, 0.95);
    --btn-play-active-secondary: rgba(225, 120, 150, 0.95);
    
    /* Button Colors - Expand - Peach Glow */
    --btn-expand: rgba(255, 180, 140, 0.95);
    --btn-expand-secondary: rgba(245, 170, 130, 0.95);
    --btn-expand-hover: rgba(255, 200, 160, 0.98);
    --btn-expand-hover-secondary: rgba(255, 190, 150, 0.98);
    --btn-expand-active: rgba(235, 160, 120, 0.95);
    --btn-expand-active-secondary: rgba(225, 150, 110, 0.95);
    
    /* Progress Bar Colors */
    --progress-bg: rgba(60, 60, 80, 0.25);
    --progress-fill-start: rgba(255, 150, 180, 0.95);
    --progress-fill-end: rgba(255, 180, 140, 0.95);
    
    /* Slider Handle Colors */
    --handle-color: rgba(255, 150, 180, 0.98);
    --handle-hover: rgba(255, 170, 195, 1.0);
    --handle-border: rgba(235, 130, 160, 0.5);
    --handle-shadow: rgba(255, 150, 180, 0.5);
    
    /* Text Colors - Soft on dark */
    --text-primary: rgba(250, 240, 245, 0.95);
    --text-secondary: rgba(220, 205, 215, 0.85);
    --text-tertiary: rgba(180, 165, 180, 0.75);
    --text-muted: rgba(140, 125, 145, 0.65);
    
    /* Border Colors */
    --border-primary: rgba(90, 85, 100, 0.35);
    --border-button: rgba(80, 75, 90, 0.3);
    --border-button-hover: rgba(110, 105, 120, 0.45);
    --border-play: rgba(255, 150, 180, 0.4);
    --border-play-hover: rgba(255, 170, 195, 0.55);
    --border-expand: rgba(255, 180, 140, 0.4);
    --border-expand-hover: rgba(255, 200, 160, 0.55);
    
    /* Shadow Colors */
    --shadow-default: rgba(0, 0, 0, 0.4);
    --shadow-button: rgba(0, 0, 0, 0.3);
    --shadow-play: rgba(255, 150, 180, 0.4);
    --shadow-play-hover: rgba(255, 170, 195, 0.6);
    --shadow-expand: rgba(255, 180, 140, 0.4);
    --shadow-expand-hover: rgba(255, 200, 160, 0.6);
    --shadow-focus: rgba(255, 150, 180, 0.6);
    
    /* Spacing & Sizes */
    --border-radius-container: 100px;
    --border-radius-section: 20px;
    --border-radius-album: 16px;
    --border-radius-button: 50%;
    --border-radius-progress: 2px;
    
    --padding-container: 12px;
    --padding-section: 16px;
}

/* Visualizer bars - pink to peach sunset gradient */
.visualizer-bar {
    background: linear-gradient(180deg, 
        rgba(255, 170, 195, 0.98),
        rgba(255, 200, 160, 0.98));
    border-radius: 0px;
    transition: all 0.05s ease-out;
    margin: 0px;
    min-width: 1px;
    min-height: 3px;
    box-shadow: 0 0 10px rgba(255, 150, 180, 0.5);
}
```

## Lucid Dreaming

<img width="664" height="372" alt="image" src="https://github.com/user-attachments/assets/ef3b376c-dcaa-4412-bdee-0f669ef5e8da" />


```css
/* ========================================
   HyprWave - Lucid Dreaming Theme
   Dark base with vibrant pink/purple/cyan retro vibes
   Based on 80s/90s nostalgia aesthetic
   ======================================== */

:root {
    /* Background Colors - Vintage Dark */
    --bg-primary: rgba(35, 30, 50, 0.75);
    --bg-secondary: rgba(40, 35, 55, 0.75);
    --bg-album-cover: rgba(45, 40, 60, 0.85);
    --bg-album-secondary: rgba(50, 45, 65, 0.85);
    
    /* Button Colors - Default (Prev/Next) - Purple Base */
    --btn-default: rgba(120, 80, 140, 0.85);
    --btn-default-secondary: rgba(110, 70, 130, 0.85);
    --btn-default-hover: rgba(140, 100, 160, 0.92);
    --btn-default-hover-secondary: rgba(130, 90, 150, 0.92);
    
    /* Button Colors - Play/Pause - Hot Pink */
    --btn-play: rgba(255, 80, 150, 0.95);
    --btn-play-secondary: rgba(245, 70, 140, 0.95);
    --btn-play-hover: rgba(255, 100, 170, 0.98);
    --btn-play-hover-secondary: rgba(255, 90, 160, 0.98);
    --btn-play-active: rgba(235, 60, 130, 0.95);
    --btn-play-active-secondary: rgba(225, 50, 120, 0.95);
    
    /* Button Colors - Expand - Cyan Accent */
    --btn-expand: rgba(100, 220, 255, 0.95);
    --btn-expand-secondary: rgba(90, 210, 245, 0.95);
    --btn-expand-hover: rgba(120, 240, 255, 0.98);
    --btn-expand-hover-secondary: rgba(110, 230, 255, 0.98);
    --btn-expand-active: rgba(80, 200, 235, 0.95);
    --btn-expand-active-secondary: rgba(70, 190, 225, 0.95);
    
    /* Progress Bar Colors */
    --progress-bg: rgba(60, 50, 80, 0.25);
    --progress-fill-start: rgba(255, 80, 150, 0.95);
    --progress-fill-end: rgba(100, 220, 255, 0.95);
    
    /* Slider Handle Colors */
    --handle-color: rgba(255, 80, 150, 0.98);
    --handle-hover: rgba(255, 100, 170, 1.0);
    --handle-border: rgba(235, 60, 130, 0.5);
    --handle-shadow: rgba(255, 80, 150, 0.6);
    
    /* Text Colors - Vibrant retro */
    --text-primary: rgba(255, 240, 250, 0.95);
    --text-secondary: rgba(230, 210, 230, 0.85);
    --text-tertiary: rgba(190, 170, 200, 0.75);
    --text-muted: rgba(150, 130, 170, 0.65);
    
    /* Border Colors */
    --border-primary: rgba(120, 80, 140, 0.35);
    --border-button: rgba(110, 70, 130, 0.3);
    --border-button-hover: rgba(140, 100, 160, 0.45);
    --border-play: rgba(255, 80, 150, 0.4);
    --border-play-hover: rgba(255, 100, 170, 0.55);
    --border-expand: rgba(100, 220, 255, 0.4);
    --border-expand-hover: rgba(120, 240, 255, 0.55);
    
    /* Shadow Colors */
    --shadow-default: rgba(0, 0, 0, 0.4);
    --shadow-button: rgba(0, 0, 0, 0.3);
    --shadow-play: rgba(255, 80, 150, 0.5);
    --shadow-play-hover: rgba(255, 100, 170, 0.7);
    --shadow-expand: rgba(100, 220, 255, 0.5);
    --shadow-expand-hover: rgba(120, 240, 255, 0.7);
    --shadow-focus: rgba(255, 150, 200, 0.6);
    
    /* Spacing & Sizes */
    --border-radius-container: 100px;
    --border-radius-section: 20px;
    --border-radius-album: 16px;
    --border-radius-button: 50%;
    --border-radius-progress: 2px;
    
    --padding-container: 12px;
    --padding-section: 16px;
}

/* Visualizer bars - retro pink to cyan gradient */
.visualizer-bar {
    background: linear-gradient(180deg, 
        rgba(255, 100, 170, 0.98),
        rgba(180, 120, 255, 0.98),
        rgba(120, 240, 255, 0.98));
    border-radius: 0px;
    transition: all 0.05s ease-out;
    margin: 0px;
    min-width: 1px;
    min-height: 3px;
    box-shadow: 0 0 12px rgba(255, 150, 200, 0.6);
}
```

## Neo-Lights

<img width="734" height="364" alt="image" src="https://github.com/user-attachments/assets/8878a99b-9f26-4cca-a35c-e0c4cacec0b0" />

```css
/* ========================================
   HyprWave - Neo-Lights Theme
   Dark base with vibrant multi-color neon accents
   Based on futuristic city aesthetic
   ======================================== */

:root {
    /* Background Colors - Deep Night */
    --bg-primary: rgba(25, 35, 45, 0.75);
    --bg-secondary: rgba(30, 40, 50, 0.75);
    --bg-album-cover: rgba(35, 45, 55, 0.85);
    --bg-album-secondary: rgba(40, 50, 60, 0.85);
    
    /* Button Colors - Default (Prev/Next) - Teal Base */
    --btn-default: rgba(60, 120, 140, 0.85);
    --btn-default-secondary: rgba(50, 110, 130, 0.85);
    --btn-default-hover: rgba(80, 140, 160, 0.92);
    --btn-default-hover-secondary: rgba(70, 130, 150, 0.92);
    
    /* Button Colors - Play/Pause - Hot Pink/Orange */
    --btn-play: rgba(255, 100, 150, 0.95);
    --btn-play-secondary: rgba(255, 140, 80, 0.95);
    --btn-play-hover: rgba(255, 120, 170, 0.98);
    --btn-play-hover-secondary: rgba(255, 160, 100, 0.98);
    --btn-play-active: rgba(235, 80, 130, 0.95);
    --btn-play-active-secondary: rgba(245, 120, 60, 0.95);
    
    /* Button Colors - Expand - Cyan Glow */
    --btn-expand: rgba(50, 200, 255, 0.95);
    --btn-expand-secondary: rgba(80, 220, 255, 0.95);
    --btn-expand-hover: rgba(70, 220, 255, 0.98);
    --btn-expand-hover-secondary: rgba(100, 240, 255, 0.98);
    --btn-expand-active: rgba(30, 180, 235, 0.95);
    --btn-expand-active-secondary: rgba(60, 200, 245, 0.95);
    
    /* Progress Bar Colors */
    --progress-bg: rgba(40, 55, 70, 0.25);
    --progress-fill-start: rgba(255, 100, 150, 0.95);
    --progress-fill-end: rgba(50, 200, 255, 0.95);
    
    /* Slider Handle Colors */
    --handle-color: rgba(255, 100, 150, 0.98);
    --handle-hover: rgba(255, 120, 170, 1.0);
    --handle-border: rgba(235, 80, 130, 0.5);
    --handle-shadow: rgba(255, 100, 150, 0.6);
    
    /* Text Colors - Bright neon */
    --text-primary: rgba(240, 250, 255, 0.95);
    --text-secondary: rgba(200, 220, 235, 0.85);
    --text-tertiary: rgba(150, 180, 210, 0.75);
    --text-muted: rgba(110, 140, 170, 0.65);
    
    /* Border Colors */
    --border-primary: rgba(60, 120, 140, 0.35);
    --border-button: rgba(50, 110, 130, 0.3);
    --border-button-hover: rgba(80, 140, 160, 0.45);
    --border-play: rgba(255, 100, 150, 0.4);
    --border-play-hover: rgba(255, 120, 170, 0.55);
    --border-expand: rgba(50, 200, 255, 0.4);
    --border-expand-hover: rgba(70, 220, 255, 0.55);
    
    /* Shadow Colors */
    --shadow-default: rgba(0, 0, 0, 0.4);
    --shadow-button: rgba(0, 0, 0, 0.3);
    --shadow-play: rgba(255, 100, 150, 0.5);
    --shadow-play-hover: rgba(255, 120, 170, 0.7);
    --shadow-expand: rgba(50, 200, 255, 0.5);
    --shadow-expand-hover: rgba(70, 220, 255, 0.7);
    --shadow-focus: rgba(255, 100, 200, 0.6);
    
    /* Spacing & Sizes */
    --border-radius-container: 100px;
    --border-radius-section: 20px;
    --border-radius-album: 16px;
    --border-radius-button: 50%;
    --border-radius-progress: 2px;
    
    --padding-container: 12px;
    --padding-section: 16px;
}

/* Visualizer bars - vibrant multi-color gradient */
.visualizer-bar {
    background: linear-gradient(180deg, 
        rgba(255, 120, 170, 0.98),
        rgba(255, 160, 100, 0.98),
        rgba(70, 220, 255, 0.98));
    border-radius: 0px;
    transition: all 0.05s ease-out;
    margin: 0px;
    min-width: 1px;
    min-height: 3px;
    box-shadow: 0 0 12px rgba(255, 100, 200, 0.6);
}
```

## Midnight City

<img width="638" height="358" alt="image" src="https://github.com/user-attachments/assets/73c41c8a-2aa3-4bc9-83bc-868da0344277" />

```css
/* ========================================
   HyprWave - Midnight City Theme
   Dark cityscape with bright blue accents
   Based on urban nightscape aesthetic
   ======================================== */

:root {
    /* Background Colors - Deep Night City */
    --bg-primary: rgba(20, 25, 30, 0.75);
    --bg-secondary: rgba(25, 30, 35, 0.75);
    --bg-album-cover: rgba(30, 35, 40, 0.85);
    --bg-album-secondary: rgba(35, 40, 45, 0.85);
    
    /* Button Colors - Default (Prev/Next) - City Lights */
    --btn-default: rgba(70, 85, 100, 0.85);
    --btn-default-secondary: rgba(60, 75, 90, 0.85);
    --btn-default-hover: rgba(90, 105, 120, 0.92);
    --btn-default-hover-secondary: rgba(80, 95, 110, 0.92);
    
    /* Button Colors - Play/Pause - Electric Blue */
    --btn-play: rgba(50, 150, 255, 0.95);
    --btn-play-secondary: rgba(40, 130, 235, 0.95);
    --btn-play-hover: rgba(70, 170, 255, 0.98);
    --btn-play-hover-secondary: rgba(60, 150, 245, 0.98);
    --btn-play-active: rgba(30, 130, 225, 0.95);
    --btn-play-active-secondary: rgba(20, 110, 205, 0.95);
    
    /* Button Colors - Expand - Amber Glow */
    --btn-expand: rgba(255, 180, 50, 0.95);
    --btn-expand-secondary: rgba(245, 160, 40, 0.95);
    --btn-expand-hover: rgba(255, 200, 70, 0.98);
    --btn-expand-hover-secondary: rgba(255, 180, 60, 0.98);
    --btn-expand-active: rgba(235, 160, 30, 0.95);
    --btn-expand-active-secondary: rgba(215, 140, 20, 0.95);
    
    /* Progress Bar Colors */
    --progress-bg: rgba(40, 50, 60, 0.25);
    --progress-fill-start: rgba(50, 150, 255, 0.95);
    --progress-fill-end: rgba(255, 180, 50, 0.95);
    
    /* Slider Handle Colors */
    --handle-color: rgba(50, 150, 255, 0.98);
    --handle-hover: rgba(70, 170, 255, 1.0);
    --handle-border: rgba(30, 130, 225, 0.5);
    --handle-shadow: rgba(50, 150, 255, 0.5);
    
    /* Text Colors - Bright on dark */
    --text-primary: rgba(240, 245, 250, 0.95);
    --text-secondary: rgba(200, 210, 220, 0.85);
    --text-tertiary: rgba(160, 180, 200, 0.75);
    --text-muted: rgba(120, 140, 160, 0.65);
    
    /* Border Colors */
    --border-primary: rgba(70, 85, 100, 0.35);
    --border-button: rgba(60, 75, 90, 0.3);
    --border-button-hover: rgba(90, 105, 120, 0.45);
    --border-play: rgba(50, 150, 255, 0.4);
    --border-play-hover: rgba(70, 170, 255, 0.55);
    --border-expand: rgba(255, 180, 50, 0.4);
    --border-expand-hover: rgba(255, 200, 70, 0.55);
    
    /* Shadow Colors */
    --shadow-default: rgba(0, 0, 0, 0.4);
    --shadow-button: rgba(0, 0, 0, 0.3);
    --shadow-play: rgba(50, 150, 255, 0.4);
    --shadow-play-hover: rgba(70, 170, 255, 0.6);
    --shadow-expand: rgba(255, 180, 50, 0.4);
    --shadow-expand-hover: rgba(255, 200, 70, 0.6);
    --shadow-focus: rgba(50, 150, 255, 0.6);
    
    /* Spacing & Sizes */
    --border-radius-container: 100px;
    --border-radius-section: 20px;
    --border-radius-album: 16px;
    --border-radius-button: 50%;
    --border-radius-progress: 2px;
    
    --padding-container: 12px;
    --padding-section: 16px;
}

/* Visualizer bars - bright city lights against dark */
.visualizer-bar {
    background: linear-gradient(180deg, 
        rgba(70, 170, 255, 0.98),
        rgba(255, 200, 70, 0.98));
    border-radius: 0px;
    transition: all 0.05s ease-out;
    margin: 0px;
    min-width: 1px;
    min-height: 3px;
    box-shadow: 0 0 8px rgba(50, 150, 255, 0.6);
}
```

## DeathStar

<img width="652" height="366" alt="image" src="https://github.com/user-attachments/assets/dd31d18f-5be3-467f-a2c2-27567df27636" />


```css
/* ========================================
   HyprWave - DeathStar Theme
   Dark smoky background with silver/cyan accents
   Based on racing/motorsport aesthetic
   ======================================== */

:root {
    /* Background Colors - Smoky Track */
    --bg-primary: rgba(30, 35, 40, 0.75);
    --bg-secondary: rgba(35, 40, 45, 0.75);
    --bg-album-cover: rgba(40, 45, 50, 0.85);
    --bg-album-secondary: rgba(45, 50, 55, 0.85);
    
    /* Button Colors - Default (Prev/Next) - Titanium */
    --btn-default: rgba(100, 110, 120, 0.85);
    --btn-default-secondary: rgba(90, 100, 110, 0.85);
    --btn-default-hover: rgba(120, 130, 140, 0.92);
    --btn-default-hover-secondary: rgba(110, 120, 130, 0.92);
    
    /* Button Colors - Play/Pause - Silver Chrome */
    --btn-play: rgba(180, 200, 220, 0.95);
    --btn-play-secondary: rgba(170, 190, 210, 0.95);
    --btn-play-hover: rgba(200, 220, 240, 0.98);
    --btn-play-hover-secondary: rgba(190, 210, 230, 0.98);
    --btn-play-active: rgba(160, 180, 200, 0.95);
    --btn-play-active-secondary: rgba(150, 170, 190, 0.95);
    
    /* Button Colors - Expand - Cyan Glow */
    --btn-expand: rgba(80, 200, 220, 0.95);
    --btn-expand-secondary: rgba(70, 190, 210, 0.95);
    --btn-expand-hover: rgba(100, 220, 240, 0.98);
    --btn-expand-hover-secondary: rgba(90, 210, 230, 0.98);
    --btn-expand-active: rgba(60, 180, 200, 0.95);
    --btn-expand-active-secondary: rgba(50, 170, 190, 0.95);
    
    /* Progress Bar Colors */
    --progress-bg: rgba(50, 55, 60, 0.25);
    --progress-fill-start: rgba(180, 200, 220, 0.95);
    --progress-fill-end: rgba(80, 200, 220, 0.95);
    
    /* Slider Handle Colors */
    --handle-color: rgba(180, 200, 220, 0.98);
    --handle-hover: rgba(200, 220, 240, 1.0);
    --handle-border: rgba(160, 180, 200, 0.5);
    --handle-shadow: rgba(180, 200, 220, 0.5);
    
    /* Text Colors - Metallic bright */
    --text-primary: rgba(240, 245, 250, 0.95);
    --text-secondary: rgba(200, 210, 220, 0.85);
    --text-tertiary: rgba(160, 175, 190, 0.75);
    --text-muted: rgba(120, 135, 150, 0.65);
    
    /* Border Colors */
    --border-primary: rgba(100, 110, 120, 0.35);
    --border-button: rgba(90, 100, 110, 0.3);
    --border-button-hover: rgba(120, 130, 140, 0.45);
    --border-play: rgba(180, 200, 220, 0.4);
    --border-play-hover: rgba(200, 220, 240, 0.55);
    --border-expand: rgba(80, 200, 220, 0.4);
    --border-expand-hover: rgba(100, 220, 240, 0.55);
    
    /* Shadow Colors */
    --shadow-default: rgba(0, 0, 0, 0.4);
    --shadow-button: rgba(0, 0, 0, 0.3);
    --shadow-play: rgba(180, 200, 220, 0.4);
    --shadow-play-hover: rgba(200, 220, 240, 0.6);
    --shadow-expand: rgba(80, 200, 220, 0.4);
    --shadow-expand-hover: rgba(100, 220, 240, 0.6);
    --shadow-focus: rgba(180, 200, 220, 0.6);
    
    /* Spacing & Sizes */
    --border-radius-container: 100px;
    --border-radius-section: 20px;
    --border-radius-album: 16px;
    --border-radius-button: 50%;
    --border-radius-progress: 2px;
    
    --padding-container: 12px;
    --padding-section: 16px;
}

/* Visualizer bars - silver to cyan gradient */
.visualizer-bar {
    background: linear-gradient(180deg, 
        rgba(200, 220, 240, 0.98),
        rgba(100, 220, 240, 0.98));
    border-radius: 0px;
    transition: all 0.05s ease-out;
    margin: 0px;
    min-width: 1px;
    min-height: 3px;
    box-shadow: 0 0 10px rgba(180, 200, 220, 0.5);
}
```

## OCTANE

For my GOAT Don Toliver, a special theme for his upcoming album, OCTANE.

<img width="629" height="351" alt="image" src="https://github.com/user-attachments/assets/452c70b4-b2e3-436b-8b43-1c5aea147d25" />

```css
/* ========================================
   HyprWave - OCTANE Theme
   Pure black and white with bright white accents
   Based on minimalist artistic aesthetic
   ======================================== */

:root {
    /* Background Colors - Deep Black */
    --bg-primary: rgba(10, 10, 10, 0.75);
    --bg-secondary: rgba(15, 15, 15, 0.75);
    --bg-album-cover: rgba(20, 20, 20, 0.85);
    --bg-album-secondary: rgba(25, 25, 25, 0.85);
    
    /* Button Colors - Default (Prev/Next) - Soft White */
    --btn-default: rgba(200, 200, 200, 0.85);
    --btn-default-secondary: rgba(190, 190, 190, 0.85);
    --btn-default-hover: rgba(220, 220, 220, 0.92);
    --btn-default-hover-secondary: rgba(210, 210, 210, 0.92);
    
    /* Button Colors - Play/Pause - Bright White */
    --btn-play: rgba(255, 255, 255, 0.95);
    --btn-play-secondary: rgba(245, 245, 245, 0.95);
    --btn-play-hover: rgba(255, 255, 255, 0.98);
    --btn-play-hover-secondary: rgba(250, 250, 250, 0.98);
    --btn-play-active: rgba(230, 230, 230, 0.95);
    --btn-play-active-secondary: rgba(220, 220, 220, 0.95);
    
    /* Button Colors - Expand - Cool White */
    --btn-expand: rgba(235, 235, 240, 0.95);
    --btn-expand-secondary: rgba(225, 225, 235, 0.95);
    --btn-expand-hover: rgba(245, 245, 250, 0.98);
    --btn-expand-hover-secondary: rgba(240, 240, 245, 0.98);
    --btn-expand-active: rgba(215, 215, 225, 0.95);
    --btn-expand-active-secondary: rgba(205, 205, 220, 0.95);
    
    /* Progress Bar Colors */
    --progress-bg: rgba(40, 40, 40, 0.25);
    --progress-fill-start: rgba(255, 255, 255, 0.95);
    --progress-fill-end: rgba(200, 200, 200, 0.95);
    
    /* Slider Handle Colors */
    --handle-color: rgba(255, 255, 255, 0.98);
    --handle-hover: rgba(255, 255, 255, 1.0);
    --handle-border: rgba(230, 230, 230, 0.5);
    --handle-shadow: rgba(255, 255, 255, 0.6);
    
    /* Text Colors - Bright white on pure black */
    --text-primary: rgba(255, 255, 255, 0.95);
    --text-secondary: rgba(230, 230, 230, 0.85);
    --text-tertiary: rgba(200, 200, 200, 0.75);
    --text-muted: rgba(160, 160, 160, 0.65);
    
    /* Border Colors */
    --border-primary: rgba(80, 80, 80, 0.35);
    --border-button: rgba(100, 100, 100, 0.3);
    --border-button-hover: rgba(120, 120, 120, 0.45);
    --border-play: rgba(255, 255, 255, 0.4);
    --border-play-hover: rgba(255, 255, 255, 0.55);
    --border-expand: rgba(235, 235, 240, 0.4);
    --border-expand-hover: rgba(245, 245, 250, 0.55);
    
    /* Shadow Colors */
    --shadow-default: rgba(0, 0, 0, 0.5);
    --shadow-button: rgba(0, 0, 0, 0.4);
    --shadow-play: rgba(255, 255, 255, 0.5);
    --shadow-play-hover: rgba(255, 255, 255, 0.7);
    --shadow-expand: rgba(235, 235, 240, 0.5);
    --shadow-expand-hover: rgba(245, 245, 250, 0.7);
    --shadow-focus: rgba(255, 255, 255, 0.7);
    
    /* Spacing & Sizes */
    --border-radius-container: 100px;
    --border-radius-section: 20px;
    --border-radius-album: 16px;
    --border-radius-button: 50%;
    --border-radius-progress: 2px;
    
    --padding-container: 12px;
    --padding-section: 16px;
}

/* Visualizer bars - pure white with strong glow */
.visualizer-bar {
    background: linear-gradient(180deg, 
        rgba(255, 255, 255, 0.98),
        rgba(230, 230, 230, 0.98));
    border-radius: 0px;
    transition: all 0.05s ease-out;
    margin: 0px;
    min-width: 1px;
    min-height: 3px;
    box-shadow: 0 0 15px rgba(255, 255, 255, 0.8),
                0 0 8px rgba(255, 255, 255, 0.6);
}
```

For people feeling like making their own themes and wanting to share it to the world, make sure to follow the above formatting of themes, and I will make sure to merge them!

# Community Themes

Empty for now!

