#include "vim_registers.h"

#include <QClipboard>
#include <QGuiApplication>

namespace Ui {
namespace Vim {

namespace {

/**
 * @brief Регистр, в который попадает всё скопированное и удалённое
 */
const QChar kUnnamedRegisterName = '"';

/**
 * @brief Регистр, в который попадает последний скопированный фрагмент
 */
const QChar kYankRegisterName = '0';

/**
 * @brief Регистр, в который попадает последнее небольшое удаление
 */
const QChar kSmallDeleteRegisterName = '-';

/**
 * @brief Регистр, из которого удалённый текст никогда не вернуть
 */
const QChar kBlackHoleRegisterName = '_';

/**
 * @brief Регистры системного буфера обмена
 */
bool isClipboardRegister(QChar _name)
{
    return _name == '+' || _name == '*';
}

/**
 * @brief Добавить перевод строки в конец построчного фрагмента
 */
QString normalizedText(const QString& _text, bool _linewise)
{
    if (!_linewise || _text.endsWith("\n")) {
        return _text;
    }

    return _text + "\n";
}

} // namespace


bool RegisterContent::isEmpty() const
{
    return text.isEmpty();
}

QChar Registers::unnamedRegister()
{
    return kUnnamedRegisterName;
}

bool Registers::isValidName(QChar _name)
{
    return _name.isLetterOrNumber() || isClipboardRegister(_name) || _name == kUnnamedRegisterName
        || _name == kBlackHoleRegisterName || _name == kSmallDeleteRegisterName;
}

void Registers::setYankedContent(QChar _name, const RegisterContent& _content)
{
    if (_name == kBlackHoleRegisterName) {
        return;
    }

    if (_name.isNull() || _name == kUnnamedRegisterName) {
        setContent(kYankRegisterName, _content);
        setContent(kUnnamedRegisterName, _content);
        syncToClipboard(_content);
        return;
    }

    setContent(_name, _content);
    if (!isClipboardRegister(_name)) {
        setContent(kUnnamedRegisterName, _content);
    }
}

void Registers::setDeletedContent(QChar _name, const RegisterContent& _content)
{
    if (_name == kBlackHoleRegisterName) {
        return;
    }

    if (_name.isNull() || _name == kUnnamedRegisterName) {
        //
        // ... удаления в пределах строки складываются отдельно от построчных
        //
        if (_content.linewise) {
            //
            // ... сдвигаем историю построчных удалений
            //
            for (int index = 9; index > 1; --index) {
                const auto previous = m_registers.value(QChar('0' + index - 1));
                if (!previous.isEmpty()) {
                    m_registers[QChar('0' + index)] = previous;
                }
            }
            setContent('1', _content);
        } else {
            setContent(kSmallDeleteRegisterName, _content);
        }

        setContent(kUnnamedRegisterName, _content);
        syncToClipboard(_content);
        return;
    }

    setContent(_name, _content);
    if (!isClipboardRegister(_name)) {
        setContent(kUnnamedRegisterName, _content);
    }
}

RegisterContent Registers::content(QChar _name) const
{
    if (_name == kBlackHoleRegisterName) {
        return {};
    }

    if (isClipboardRegister(_name)) {
        return { QGuiApplication::clipboard()->text(), false };
    }

    if (_name.isNull() || _name == kUnnamedRegisterName) {
        return unnamedContent();
    }

    return m_registers.value(_name.toLower());
}

void Registers::setContent(QChar _name, const RegisterContent& _content)
{
    if (isClipboardRegister(_name)) {
        syncToClipboard(_content);
        return;
    }

    auto content = _content;
    content.text = normalizedText(content.text, content.linewise);

    //
    // ... заглавная буква в имени регистра означает, что текст нужно дописать к уже имеющемуся
    //
    if (_name.isUpper()) {
        const auto existingContent = m_registers.value(_name.toLower());
        if (!existingContent.isEmpty()) {
            content.linewise = existingContent.linewise || content.linewise;
            content.text
                = normalizedText(existingContent.text, existingContent.linewise) + content.text;
            content.text = normalizedText(content.text, content.linewise);
        }
    }

    m_registers[_name.toLower()] = content;
}

void Registers::syncToClipboard(const RegisterContent& _content)
{
    m_clipboardText = normalizedText(_content.text, _content.linewise);
    QGuiApplication::clipboard()->setText(m_clipboardText);
}

RegisterContent Registers::unnamedContent() const
{
    const auto storedContent = m_registers.value(kUnnamedRegisterName);

    //
    // ... если буфер обмена сменился в другом приложении, то вставляем именно его содержимое
    // @note Пустой буфер обмена игнорируем, иначе регистр перестанет работать в окружениях,
    //       в которых буфер обмена недоступен
    //
    const auto clipboardText = QGuiApplication::clipboard()->text();
    if (!clipboardText.isEmpty() && clipboardText != m_clipboardText) {
        return { clipboardText, false };
    }

    return storedContent;
}

} // namespace Vim
} // namespace Ui
