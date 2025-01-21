#!/usr/bin/env python3

# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2024 Antoine Herlicq <antoine.herlicq@enioka.com>

from pathlib import Path
import tempfile
import time
import unittest

from appium import webdriver
from appium.options.common.base import AppiumOptions
from appium.webdriver.common.appiumby import AppiumBy
from selenium.webdriver import ActionChains, Keys
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import NoSuchElementException


class SimpleTextEditorTests(unittest.TestCase):
    text_area_id = "KTextEditor::ViewPrivate.KateViewInternal"

    def find_element_by_class_name(self, type, name):
        return self.driver.find_element(by=AppiumBy.CLASS_NAME, value=f"[{type} | {name}]")

    def find_element_by_name(self, element):
        return self.driver.find_element(by=AppiumBy.NAME, value=element)

    def find_element_by_accessibility_id(self, element):
        return self.driver.find_element(by=AppiumBy.ACCESSIBILITY_ID, value=element)

    def find_text_area(self):
        return self.find_element_by_accessibility_id(self.text_area_id)

    def find_main_window(self):
        return self.find_element_by_accessibility_id("KateCentralWidget")

    def new_file_button(self):
        return self.find_element_by_name("New File")

    def write_text(self, area_to_write, text):
        actions = ActionChains(self.driver)
        actions.move_to_element(area_to_write)
        actions.click()
        actions.send_keys(text)
        actions.perform()

    def write_text_slow(self, area_to_write, text):
        actions = ActionChains(self.driver)
        actions.move_to_element(area_to_write)
        actions.click().perform
        for l in text:
            actions = ActionChains(self.driver)
            actions.send_keys(l).perform()


    def wait_for_text(self, text):
        self.wait.until(EC.text_to_be_present_in_element((AppiumBy.ACCESSIBILITY_ID, self.text_area_id), text))

    def setUp(self):
        options = AppiumOptions()
        options.set_capability("app", "org.kde.kwrite.desktop")

        max_attempts = 3
        attempts = 0
        while attempts < max_attempts:
            try:
                self.driver = webdriver.Remote(command_executor='http://127.0.0.1:4723', options=options)
                break
            except Exception as e:
                attempts += 1
                time.sleep(1)
        self.driver.implicitly_wait = 10
        self.wait = WebDriverWait(self.driver, 10)

    def tearDown(self):
        max_attempts = 3
        attempts = 0
        while attempts < max_attempts:
            try:
                self.driver.quit()
                break
            except Exception as e:
                attempts += 1
                time.sleep(1)

    def test_a_app_installed(self):
        self.assertTrue(self.driver.is_app_installed("kwrite"))

    def test_b_app_runs(self):
        self.assertTrue(self.new_file_button().is_displayed())
        self.find_element_by_name("New File").click()
        self.assertTrue(self.find_element_by_class_name("frame", "Untitled "))

    def test_c_writing_text(self):
        self.new_file_button().click()
        main = self.find_main_window()
        self.write_text(main, "Hello, World!")
        self.wait_for_text("Hello, World!")
        text_area = self.find_text_area()
        self.assertEqual(text_area.text, "Hello, World!")

    def test_d_undo_redo_buttons(self):
        self.new_file_button().click()
        main = self.find_main_window()
        self.write_text(main, "Hello," + Keys.ENTER + "World!")
        self.wait_for_text("Hello,\nWorld!")
        text_area = self.find_text_area()
        self.assertEqual(text_area.text, "Hello,\nWorld!")
        self.find_element_by_name("Undo").click()
        self.assertEqual(text_area.text, "Hello,\n")
        self.find_element_by_name("Redo").click()
        self.assertEqual(text_area.text, "Hello,\nWorld!")

    def test_e_new_tab(self):
        self.new_file_button().click()
        main = self.find_main_window()
        self.write_text(main, "Hello, World!")
        self.wait_for_text("Hello, World!")
        text_area = self.find_text_area()
        self.assertEqual(text_area.text, "Hello, World!")
        self.find_element_by_name("New").click()
        self.assertFalse(text_area.is_displayed())

        self.write_text(main, "Bye, Moon!")
        self.wait_for_text("Bye, Moon!")
        text_area2 = self.find_text_area()
        self.assertEqual(text_area2.text, "Bye, Moon!")

        text_area2 = self.find_text_area()
        self.find_element_by_class_name("page tab", "Untitled").click()
        self.assertTrue(text_area.is_displayed())
        self.assertFalse(text_area2.is_displayed())

        self.find_element_by_class_name("menu item", "File").click()
        self.find_element_by_class_name("menu item", "Close").click()
        self.driver.find_element(by="description", value="Discard changes").click()
        self.wait.until(EC.visibility_of_element_located((AppiumBy.ACCESSIBILITY_ID, self.text_area_id)))

        self.assertFalse(text_area.is_displayed())
        self.assertTrue(text_area2.is_displayed())

    def test_f_find_and_replace(self):
        self.new_file_button().click()
        main = self.find_main_window()
        self.write_text(main, "Hello, World!")
        self.wait_for_text("Hello, World!")
        text_area = self.find_text_area()
        self.assertEqual(text_area.text, "Hello, World!")

        self.find_element_by_class_name("menu item", "Edit").click()
        self.find_element_by_accessibility_id("edit.edit_replace").click()

        find_input = self.find_element_by_class_name("combo box", "Find:")
        self.write_text(find_input, "Hello" + Keys.TAB + "Good Morning")
        self.wait.until(EC.text_to_be_present_in_element((AppiumBy.ACCESSIBILITY_ID, "PowerSearchBar.replacement"), "Good Morning"))

        self.find_element_by_class_name("button", "Replace All").click()
        self.wait_for_text("Good Morning, World!")
        self.assertEqual(text_area.text, "Good Morning, World!")

    def test_g_save_file(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            file_path = Path(temp_dir) / "test_file"
            self.new_file_button().click()
            main = self.find_main_window()
            self.write_text(main, "Hello, World!")
            self.wait.until(EC.text_to_be_present_in_element((AppiumBy.ACCESSIBILITY_ID, self.text_area_id), "Hello, World!"))
            text_area = self.find_text_area()
            self.assertEqual(text_area.text, "Hello, World!")

            self.find_element_by_class_name("button", "Save As…").click()

            save_dialog = self.find_element_by_class_name("dialog", "Save File")
            name_area_save = self.find_element_by_class_name("text", "")
            self.write_text_slow(name_area_save, str(file_path))
            save_dialog.find_element(by=AppiumBy.NAME, value="Save").click()

            # Wait for the file to be written
            wait_start = time.time()
            while time.time() - wait_start < 3 and not file_path.is_file():
                time.sleep(1)
            if not file_path.is_file():
                raise AssertionError("File was not saved")

            self.find_element_by_name("New").click()
            self.find_element_by_class_name("page tab", "test_file").click()
            self.find_element_by_class_name("menu item", "File").click()
            self.find_element_by_class_name("menu item", "Close").click()
            # Issue with the previous action "Close" that does not close the menu.
            self.find_element_by_class_name("menu item", "File").click()
            # We click again on "File" to close it.
            self.find_element_by_class_name("menu item", "File").click()

            self.find_element_by_class_name("menu item", "Open…").click()
            name_area_open = self.find_element_by_class_name("text", "")
            self.write_text_slow(name_area_open, str(file_path))
            self.find_element_by_class_name("button", "Open").click()
            text_area = self.find_text_area()
            self.assertEqual(text_area.text, "Hello, World!\n")  # There's an extra line when we open a file

if __name__ == '__main__':
    unittest.main()
