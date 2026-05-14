This system creates a realistic character <-> environment interaction pipeline in Unreal Engine 5, where characters dynamically leave footprints on the landscape that respond to surface wetness and material type.

The core mechanic is a render target that follows the character, persistently storing footprint data in world space. When a character walks over a wet area, the system reads the exact wetness level of the landscape layer at the foot's position using a custom C++ Blueprint Function Library, giving a continuous 0 to 1 value rather than a simple binary wet/dry detection. This wetness level determines how strongly the feet get coated, and progressively decreases as the character walks on dry land.

The system is designed with artist workflow as a priority. A single Data Table drives all per-surface behavior, artists define the normal perturbation intensity for both dry and wet states, assign Niagara particle systems for dry and wet impacts, and tune all parameters without touching any Blueprint logic. The wet Niagara effect is additionally tied to the current wetness level, so the visual response scales naturally with the intensity of the surface.

On the material side, a single footprint texture is sufficient. The system automatically handles left and right foot flipping, orients footprints to the character's walking direction, and samples the landscape material underneath, meaning footprints inherit the correct surface appearance without requiring per material texture variants.

https://youtu.be/RW6g-0-DFy8
