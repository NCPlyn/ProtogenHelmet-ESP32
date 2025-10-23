# Common Problems and Fixes

## General Setup

* **No spaces in paths:**
  Ensure your user folder and project path do not contain spaces.
  Example:  
  ✅ `C:/Users/JohnDoe/Projects/Controller`  
  ❌ `C:/Users/John Doe/Projects/Controller`

* **VS Code installation:**

  1. Install Visual Studio Code.
  2. Install the **PlatformIO** extension.
  3. Wait for PlatformIO to finish all initial installations.
  4. Restart VS Code.
  5. Open the project folder (`Controller` or `Remote`).
  6. Wait again for PlatformIO to finish setting up.
  7. Watch for any errors before proceeding.

* **Upload order:**
  Always upload in this order:

  1. **Upload code** (`Upload`)
  2. **Upload filesystem** (`Upload Filesystem`)

---

## Upload Issues in VS Code

If you cannot upload due to an error:

1. Restart VS Code.
2. If a library is missing, manually correct the import or code where the error appears.
3. Yellow warnings can usually be ignored (especially "`xxx` was redefined").
4. If problems persist:
   * Uninstall PlatformIO from VS Code.
   * Delete the `.platformio` folder.
   * Reinstall PlatformIO and repeat setup.
5. Verify your board settings in `platformio.ini`:
   * Make sure you're using an **ESP32-S3 with PSRAM**.
   * Select the correct board variant (e.g. `esp32-s3-devkitc-n8r2`).

---

## After Upload: Functionality Issues

If both code and filesystem were uploaded but something doesn’t work:

* Open the **PlatformIO Monitor** and check for error messages.
* If you see `"file not found"` in the monitor or on `192.168.4.1`, it means:
  * The filesystem was **not uploaded** successfully.

---

## Display or Hardware Issues

If displays glitch or behave incorrectly:

1. Use proper cables:
   * Minimum **0.25mm² / 22 AWG**, **>95% copper** quality.
2. Check wiring:
   * Ensure all connections match the pinout.
3. Verify grounding and power:
   * All grounds (`-`) must be properly connected.
   * Make sure the power supply provides stable voltage without drops.
4. Swap LED matrices or modules if problems persist:
   * Some **cheap MAX clones** are unreliable or faulty across batches.
