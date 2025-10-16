## Project Overview

This project is a Java-based library management system. It is built using Apache Maven and follows standard Java best practices. The goal is to create a simple, robust, and maintainable application for managing books, members, and loans in a library.

## Tech Stack

- **Language:** Java
- **Build Tool:** Apache Maven
- **Testing:** JUnit 5

## Project Structure

Please adhere to the standard Maven project structure:

- `src/main/java`: Main application source code.
- `src/main/resources`: Main application resources (e.g., configuration files, property files).
- `src/test/java`: Test source code.
- `src/test/resources`: Test resources (e.g., test data, configuration files).
- `pom.xml`: Maven project configuration.

## Coding Standards

### General Principles

- **Readability:** Write clear, self-documenting code. Use meaningful names for variables, methods, and classes.
- **Consistency:** Follow the existing coding style and conventions in the project.
- **Comments:** Use inline comments to clarify complex logic. Use Javadoc for all public classes and methods.
- **SOLID Principles:** Strive to follow SOLID design principles for maintainable and extensible code.
- **Error Handling:** Use structured exception handling. Avoid catching generic `Exception`. Be specific about the exceptions you catch and handle them appropriately.

### Naming Conventions

- **Classes & Interfaces:** PascalCase (e.g., `BookService`, `MemberRepository`)
- **Methods & Variables:** camelCase (e.g., `addBook`, `bookTitle`)
- **Constants:** UPPER_CASE_SNAKE_CASE (e.g., `MAX_BOOKS_PER_MEMBER`)

### Code Style

- **Braces:** Use Allman style (braces on new lines).
- **Indentation:** Use 4 spaces per indentation level.
- **Line Length:** Limit lines to 120 characters.
- **Imports:** Organize imports and remove unused ones.

## Testing

- **Test Naming:** Use descriptive test method names: `methodName_StateUnderTest_ExpectedBehavior`.
- **Test Structure:** Follow the Arrange-Act-Assert (AAA) pattern in all tests.
- **Isolation:** Each test should be independent and not rely on the outcome of other tests.
- **Mocking:** Use mocks or fakes for external dependencies to ensure tests are deterministic.

## Dependencies

- **`pom.xml`:** Keep the `pom.xml` file clean and organized.
- **Dependency Scope:** Use the correct dependency scope (e.g., `compile`, `test`, `provided`).
- **Versioning:** Use specific versions for dependencies. Avoid using version ranges.

## Copilot Usage

- **Adhere to these standards** when generating or modifying code.
- **Prefer existing patterns** and conventions found in the codebase.
- **Generate code that is ready to use** and fits seamlessly into the current structure.
- **Document any deviations** from these standards in pull requests or code reviews.
